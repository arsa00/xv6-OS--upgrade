#ifndef _rbt_h_
#define _rbt_h_

#include "param.h"

#define N NPROC		// num of processes
#define N_trees NCPU	// num of CPUs

#define RED -1
#define BLACK 1
#define DOUBLE_BLACK -2
#define RECOLOR -1


#define NULL 0

typedef struct Node
{
	struct Node* left, * right, * parent;
	void* element;
    unsigned int value;
    int memIndex;
	short color;
} Node;

typedef struct FreeMem
{
	int index;
	struct FreeMem* next, *prev;
} FreeMem;

typedef struct RBTree
{
	Node nodes[N];
	Node* root;
	int size;
	FreeMem	*free;
	FreeMem memory[N];
}RBTree;

/*
	- isNodeValid: koristi se sa funkcijama koje vracaju Node po vrednosti.
	- funkcije koje vracaju Node kao pokazivac imaju konzistentnu vrednost dokle god se ne iskorsiti removeNode f-ja. 
	  Nakon toga ne postoje garancije vrednosti elementa na koji ukazuje pokazivac.
	- getMin i getMax samo vracaju minimalni/maksimalni element u  stablu. (nema uklanjanja, za razliku od removeMin/Max)
	- create_tree kreira stablo, inicijalizuje pocetne promenljive i vraca pokazivac na stablo. NULL ako nema mesta za novo stablo.
*/

RBTree* create_tree();
int insertNode(RBTree* tree, void* element, unsigned int priority);
Node* getMin(RBTree* tree);
Node* getMax(RBTree* tree);
int removeNode(RBTree* tree, Node* node);
Node removeMin(RBTree* tree);
Node removeMax(RBTree* tree);
Node* search(RBTree* tree, void* element, unsigned int priority);
Node* successor(Node* node);
int isNodeValid(Node node); // valid == 1

#endif