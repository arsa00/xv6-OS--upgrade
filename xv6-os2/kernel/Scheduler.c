#include "RedBlackTree.h"
#include "types.h"
#include "param.h"

#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#include "Scheduler.h"

struct ChosenScheduler
{
    getPtr get;
    putPtr put;
} chosenSched;

struct GlobalScheduler Scheduler;

#define cpu_cnt NCPU	// broj CPU-va

long total_processes[cpu_cnt], waiting_processes[cpu_cnt];
int empty_cpu, overloaded_cpu, balanced_cpu;
static int reset_counting = 0;

short SJF_alpha; // [0 - 100]
short SJF_type; // preemptive/non-preemptive

RBTree* waitingQueue[cpu_cnt];

struct proc* CFS_get(int cpu_id)
{
    struct proc *ret = (struct proc *)removeMin(waitingQueue[cpu_id]).element;
	waiting_processes[cpu_id]--;
	// set timeSlice for process...
	// koliko je dugo cekano na CPU (u vidu time slice-ova) podeljeno sa preostalim brojem procesa koji cekaju na CPU
	ret->tau = ticks - ret->tau;  // racunanje koliko je dugo proces cekao na CPU
	if(waiting_processes[cpu_id] != 0) ret->timeSlice = ret->tau / waiting_processes[cpu_id];
    else ret->timeSlice = ret->tau;
	if (ret->timeSlice == 0) ret->timeSlice = 1; // ako je previse procesa u redu cekanja stavi timeSlice na podrazumevanu vrednost
	
	return ret;
}

void CFS_put(struct proc* process, short wasBlocked)
{
	// zapamtimo vreme ulaska u red cekanja za CPU
	process->tau = ticks;

	// calculate new priority...
	insertNode(waitingQueue[process->affinity], process, process->measuredTime);  // vreme izvrsavanja (measuredTime) je prioritet za CFS
	waiting_processes[process->affinity]++;
	total_processes[process->affinity]++;
}

struct proc* SJF_get(int cpu_id)
{
    struct proc* ret = (struct proc *)removeMin(waitingQueue[cpu_id]).element;
	waiting_processes[cpu_id]--;

	// set timeSlice for process...
	if (SJF_type == PREEMPTIVE) ret->timeSlice = 1; // za SJF se odredi neki fiksan (uvek isti) timeSlice (npr. uvek 1)
	else ret->timeSlice = 0;  // ako je bez preuzimanja onda je timeSlice neogranicen (== 0)
	
	return ret;
}

void SJF_put(struct proc* process, short wasBlocked)
{
	// calculate new priority...
	if (wasBlocked)
	{
		process->tau = ((process->measuredTime << 7) * SJF_alpha + (100 - SJF_alpha)*process->tau)/100; // (measuredTime*128*alfa + (100 - alfa)*tau)/100
		process->measuredTime = 0;
	}

	insertNode(waitingQueue[process->affinity], process, process->tau);  // proracunato tau je prioritet za SJF
	waiting_processes[process->affinity]++;
	total_processes[process->affinity]++;
}

int set_scheduling_algorithm(int algorithm, short alpha, short algorithm_type) // sys_call
{
    acquire(&Scheduler.mutex);
	switch (algorithm)
	{
		case CFS:
            chosenSched.get = CFS_get; chosenSched.put = CFS_put;
            break;
		case SJF:
            if(alpha < 0 || alpha > 100 || (algorithm_type != PREEMPTIVE && algorithm_type != NON_PREEMPTIVE))
            {
                release(&Scheduler.mutex);
                return -1;
            }
            chosenSched.get = SJF_get; chosenSched.put = SJF_put; SJF_alpha = alpha; SJF_type = algorithm_type;
            break;
        default:
        {
            release(&Scheduler.mutex);
            return -1;
        }
	}

	//transition --> podesava prioritete svih procesa na podrazumevanu vrednost novoizabranog algoritma
	Node removed[NPROC];   // max number of processes (nodes) 
	
	for(int i = 0; i < cpu_cnt; i++)
	{
		int n = waitingQueue[i]->size;
		for(int j = 0; j < n; j++)
		{
			removed[j] = removeMin(waitingQueue[i]);
		}

		if (algorithm == CFS)
		{
			for (int j = 0; j < n; j++)
			{
                struct proc* p = (struct proc*)removed[j].element;
				p->tau = ticks;
				insertNode(waitingQueue[p->affinity], p, p->measuredTime);  // vreme izvrsavanja (measuredTime) je prioritet za CFS
			}
		}
		else
		{
			for (int j = 0; j < n; j++)
			{
                struct proc* p = (struct proc*)removed[j].element;
				p->tau = 0;
				insertNode(waitingQueue[p->affinity], p, p->tau);  // proracunato tau je prioritet za SJF
			}
		}

	}

    release(&Scheduler.mutex);
    return 0;
}

int calculate_priority(int cpu_id)
{
	if (waiting_processes[cpu_id] == 0) return -1;
	else if (total_processes[cpu_id] - waiting_processes[cpu_id] >= waiting_processes[cpu_id]) return 1;
	else return 0;

	/* uproscavanje --> izbaciti deljenje
	return waiting_processes[cpu_id] > 0 ? 
		(total_processes[cpu_id] - waiting_processes[cpu_id]) / waiting_processes[cpu_id] : -1;
	*/
}

void update_cpu_state(int cpu_id)
{
	if (waiting_processes[cpu_id] == 0)
	{ // prazan cpu
		if (cpus[cpu_id].state != EMPTY)
		{
			if (cpus[cpu_id].state == BALANCED) balanced_cpu--;
			else overloaded_cpu--;

			cpus[cpu_id].state = EMPTY;
			empty_cpu++;
		}
	}
	else if (calculate_priority(cpu_id) != 0)
	{ // balansiran cpu
		if (cpus[cpu_id].state != BALANCED)
		{
			if (cpus[cpu_id].state == EMPTY) empty_cpu--;
			else overloaded_cpu--;

			cpus[cpu_id].state = BALANCED;
			balanced_cpu++;
		}
	}
	else if (cpus[cpu_id].state != OVERLOADED)
	{ // preopterecen cpu
		if (cpus[cpu_id].state == EMPTY) empty_cpu--;
		else balanced_cpu--;

		cpus[cpu_id].state = OVERLOADED;
		overloaded_cpu++;
	}
}

struct proc* globalGet(int cpu_id)
{
	if (waiting_processes[cpu_id] > 0)
	{ // nije prazan CPU

        struct proc* p = chosenSched.get(cpu_id);
		update_cpu_state(cpu_id);

		return p;
	}
	else
	{ // ako jeste prazan CPU, probaj da pomognes nekom drugom CPU-u (po prioritetu)
		if (overloaded_cpu)
		{ // ako ima preoptereceni CPU-a, uzmi od onog koji ima najvise u redu za cekanje
			int id, max = -1;
			for (int i = 0; i < cpu_cnt; i++)
			{
				if (cpus[i].state == OVERLOADED &&  waiting_processes[i] > max)
				{
					max = waiting_processes[i];
					id = i;
				}
			}

            struct proc* p = chosenSched.get(id);
			update_cpu_state(id);
			p->affinity = cpu_id;

			return p;
		}
		else if (balanced_cpu)
		{ // ako nema preopterecenih CPU-a, proveri za balansirane i opet, ako ima takvog, uzmi od onog koji ima najvise u redu za cekanje
			int id, max = -1;
			for (int i = 0; i < cpu_cnt; i++)
			{
				if (cpus[i].state == BALANCED && waiting_processes[i] > max)
				{
					max = waiting_processes[i];
					id = i;
				}
			}

            struct proc* p = chosenSched.get(id);
			update_cpu_state(id);
			p->affinity = cpu_id;

			return p;
		}
	}

    return 0;
}

void globalPut(struct proc *process, short wasBlocked)
{ // ovaj globlani scheduler treba da se brine o afinitetima i load-balancing-u
	reset_counting++;
	if (reset_counting == LOAD_BALANCING_THRESHOLD)
	{
		reset_counting = 0;
		for (int i = 0; i < cpu_cnt; i++)
		{ // postavi sve procesore u BALANCED stanje, kako bi im dao sansu da se oporave
			if (waiting_processes[i] == 0)
            {
                total_processes[i] = 0;
                continue;
            }
			total_processes[i] = (waiting_processes[i] + 1) << 1;
			update_cpu_state(i);
		}
	}
	// if (p->state == SLEEPING || p->state == ZOMBIE) blocked = 1;
	if (process->affinity != -1)
    {
		if (calculate_priority(process->affinity) != 0)
        {  // balansiran ili prazan cpu
            chosenSched.put(process, wasBlocked);
            update_cpu_state(process->affinity);
        }
		else if (empty_cpu || balanced_cpu) process->affinity = -1; // ako je preopterecen cpu, proveriti ima li neki koji nije; ako ima ==> ponisti afinitet
		else
        { // ako su svi preoptereceni, bira se cpu po afinitetu
            chosenSched.put(process, wasBlocked);
            update_cpu_state(process->affinity);
        }
	}

	if (process->affinity == -1)
	{
		if (empty_cpu)
		{ // ako ima praznih procesora, ubacujemo prvo u njih
			for (int i = 0; i < cpu_cnt; i++)
			{ // nalazimo prvi prazan
				if (cpus[i].state == EMPTY)
				{
					process->affinity = i;
					break;
				}
			}
		}
		else if (balanced_cpu)
		{ // ako nema praznih, sledece trazimo balansirane procesore; ako ima takvih, biramo onaj sa najmanje procesa u redu cekanja
			int min = -1, mini = -1;
			for (int i = 0; i < cpu_cnt; i++)
			{
				if (cpus[i].state == BALANCED && (min == -1 || waiting_processes[i] < min))
				{
					min = waiting_processes[i];
					mini = i;
				}
			}
			process->affinity = mini;
		}
		else
		{ // ako nema ni praznih ni balansiranih, uzimamo neki koji je opterecen; opet biramo onaj sa min procesa u redu cekanja
			int min = -1, mini = -1;
			for (int i = 0; i < cpu_cnt; i++)
			{
				if (cpus[i].state == OVERLOADED && (min == -1 || waiting_processes[i] < min))
				{
					min = waiting_processes[i];
					mini = i;
				}
			}
			process->affinity = mini;
		}

		chosenSched.put(process, wasBlocked);
		update_cpu_state(process->affinity);
	}
}

void scheduler_init()
{ // podrazumevane vrednosti
	Scheduler.get = globalGet;
	Scheduler.put = globalPut;
	chosenSched.get = CFS_get;
	chosenSched.put = CFS_put;
	SJF_alpha = 50;
	SJF_type = PREEMPTIVE;

	balanced_cpu = empty_cpu = overloaded_cpu = 0;
	for (int i = 0; i < cpu_cnt; i++)
	{
		total_processes[i] = waiting_processes[i] = 0;
		cpus[i].state = EMPTY;
		cpus[i].myId = i;

		empty_cpu++;
		waitingQueue[i] = create_tree();
	}
}