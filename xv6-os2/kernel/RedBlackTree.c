#include "RedBlackTree.h"

static int num_of_trees = 0;
RBTree trees[N_trees];



RBTree* create_tree()
{
	//RBTree tree;
	if (num_of_trees == N_trees) return NULL;

	trees[num_of_trees].size = 0;
	for (int i = 0; i < N; i++)
	{
		trees[num_of_trees].memory[i].index = i;
		if (i > 0) trees[num_of_trees].memory[i].prev = &trees[num_of_trees].memory[i - 1];
		if(i < N-1) trees[num_of_trees].memory[i].next = &trees[num_of_trees].memory[i + 1];
	}
	trees[num_of_trees].memory[0].prev = NULL;
	trees[num_of_trees].memory[N-1].next = NULL;

	trees[num_of_trees].free = trees[num_of_trees].memory;

	return &trees[num_of_trees++];
}

void update_mem(RBTree* tree, int op, int index)
{
	if (op == 1)
	{
		// oslobi slot u memoriji
		if (tree->free != NULL)
		{
			tree->memory[index].next = tree->free;
			tree->free->prev = &tree->memory[index];
			tree->memory[index].prev = NULL;
		}
		else
		{
			tree->memory[index].next = NULL;
			tree->memory[index].prev = NULL;
		}

		tree->free = &tree->memory[index];
	}
	else
	{
		// ukloni slot u memoriji; za ovu operaciju index atribut se ne koristi;
		FreeMem* tmp = tree->free;
		tree->free = tree->free->next;

		if (tmp->prev) tmp->prev->next = tmp->next;
		if (tmp->next) tmp->next->prev = tmp->prev;
	}
}

void left_rotation(Node *node, Node **root)
{
	Node* rightChild = node->right;
	if (node != *root)
	{
		if (node->parent->left == node) node->parent->left = rightChild;
		else node->parent->right = rightChild;
	}
	else *root = rightChild;

	if (node->right)
	{
		rightChild->parent = node->parent;
		node->parent = rightChild;
		node->right = rightChild->left;
		if(rightChild->left) rightChild->left->parent = node;
		rightChild->left = node;
	}
}

void right_rotation(Node* node, Node** root)
{
	Node* leftChild = node->left;
	if (node != *root)
	{
		if (node->parent->left == node) node->parent->left = leftChild;
		else node->parent->right = leftChild;
	}
	else *root = leftChild;

	if (node->left)
	{
		leftChild->parent = node->parent;
		node->parent = leftChild;
		node->left = leftChild->right;
		if(leftChild->right) leftChild->right->parent = node;
		leftChild->right = node;
	}
}

void check_RBT(Node *node, Node **root)
{
	if (node->parent->color == BLACK) return;
	else
	{
		if (node->parent->parent->left == node->parent)
		{	// roditeljev rodjak je desni sin od dede
			if (node->parent->parent->right == NULL || (node->parent->parent->right != NULL && node->parent->parent->right->color == BLACK))
			{
				// roditelje je LEVI sin od dede
				if (node->parent->left == node)
				{
					// LL situacija
					node->parent->parent->color *= RECOLOR;
					node->parent->color *= RECOLOR;
					right_rotation(node->parent->parent, root);
				}
				else
				{
					// LR situacija
					left_rotation(node->parent, root);
					right_rotation(node->parent, root);
					node->color *= RECOLOR;
					node->right->color *= RECOLOR;
				}
			}
			else
			{
				node->parent->color *= RECOLOR;
				node->parent->parent->right->color *= RECOLOR;
				if (node->parent->parent != *root)
				{
					node->parent->parent->color *= RECOLOR;
					check_RBT(node->parent->parent, root);	// proveriti
				}
			}
		}
		else
		{	// roditeljev rodjak je levi sin od dede
			if (node->parent->parent->left == NULL || (node->parent->parent->left != NULL && node->parent->parent->left->color == BLACK))
			{
				// roditelje je DESNI sin od dede
				if (node->parent->left == node)
				{
					// RL situacija
					right_rotation(node->parent, root);
					left_rotation(node->parent, root);
					node->color *= RECOLOR;
					node->left->color *= RECOLOR;
				}
				else
				{
					// RR situacija
					node->parent->parent->color *= RECOLOR;
					node->parent->color *= RECOLOR;
					left_rotation(node->parent->parent, root);
				}
			}
			else
			{
				node->parent->color *= RECOLOR;
				node->parent->parent->left->color *= RECOLOR;
				if (node->parent->parent != *root)
				{
					node->parent->parent->color *= RECOLOR;
					check_RBT(node->parent->parent, root);	// proveriti
				}
			}
		}
	}
}

int insertNode(RBTree* tree, void* element, unsigned int priority)
{
	if (tree == NULL) return -1;
	if (tree->size == N || !tree->free) return -1;	// mozda izbaciti, ukoliko budem radio sa dinamickom memorijom

	Node newNode1;
	newNode1.left = newNode1.right = newNode1.parent = NULL;
	newNode1.color = RED;
	newNode1.element = element;
	newNode1.value = priority;
	Node *newNode = &newNode1;	// laske raditi sa pokazivacima :)

	if (!tree->size || !tree->root)	// ako je prvi element, odnosno root
	{
		newNode->color = BLACK;
		newNode->memIndex = tree->free->index;
		tree->nodes[tree->free->index] = *newNode;
		tree->root = &tree->nodes[tree->free->index];
		tree->size++;
		update_mem(tree, -1, 0); // izbaci 1 slobodan slot iz memorije
		return 0;
	}

	Node* curr = tree->root; // krenemo od root-a da trazimo mesto za unos
	//Node* curr = tree->root;

	while (1)	// u ovoj while petlji se vrsi najobicniji BST unos u stablo
	{			// sa dodatkom da ukoliko je priroritet (value) 2 Node-a isti posmatra se kao da je taj element veci (odnosno ide u desno podstablo)
		//if (curr->element == newNode->element) return -1;	// element vec postoji u listi
		if (newNode->value < curr->value && curr->left == NULL)
		{
			newNode->parent = curr;
			tree->nodes[tree->free->index] = *newNode;
			newNode = &tree->nodes[tree->free->index];
			curr->left = newNode;
			newNode->memIndex = tree->free->index;
			tree->size++;
			update_mem(tree, -1, 0); // izbaci 1 slobodan slot iz memorije
			break;
		}
		else if (newNode->value < curr->value) curr = curr->left;
		else if (curr->right == NULL)
		{
			newNode->parent = curr;
			tree->nodes[tree->free->index] = *newNode;
			newNode = &tree->nodes[tree->free->index];
			curr->right = newNode;
			newNode->memIndex = tree->free->index;
			tree->size++;
			update_mem(tree, -1, 0); // izbaci 1 slobodan slot iz memorije
			break;
		}
		else curr = curr->right;
	}

	check_RBT(newNode, &tree->root);	// check if this tree is RBT and resolves possible violences

	return 0;
}

Node* getMin(RBTree *tree)
{
	if (!tree || !tree->root) return NULL;

	Node* curr = tree->root;
	while (curr->left)	curr = curr->left;
	return curr;
}

Node* getMax(RBTree* tree)
{
	if (!tree || !tree->root) return NULL;

	Node* curr = tree->root;
	while (curr->right)	curr = curr->right;
	return curr;
}

void swap(Node* node1, Node* node2)
{
	void* tmpEl;
    unsigned int tmpVal;
	tmpEl = node1->element;
	tmpVal = node1->value;

	node1->element = node2->element;
	node1->value = node2->value;
	node2->element = tmpEl;
	node2->value = tmpVal;
}

Node* successor(Node* node)
{
	if (!node || !node->right) return NULL;
	Node* ret = NULL, *next = node->right;
	while (next) { ret = next; next = next->left; }
	return ret;
}

Node* search(RBTree *tree, void* element, unsigned int priority)
{
	if (!tree || !tree->root) return NULL;

	Node* ret = tree->root;
	while (ret && (ret->element != element || ret->value != priority))
	{
		if (priority < ret->value) ret = ret->left;
		else ret = ret->right;
	}

	return ret;
}

// --- in progress --
void removeLeaf(RBTree* t, Node* node)
{
	if (!t || !node) return;

	if (t->root == node) t->root = NULL;
	else if (node->parent->left == node) node->parent->left = NULL;
	else node->parent->right = NULL;
	update_mem(t, 1, node->memIndex); // oslobadjanje slota na kom se nalazio cvor u memoriji
}

void resolve_DB(RBTree* t, Node* node)
{
	if (!t || !node || node->color != DOUBLE_BLACK) return;

	if (t->root == node) node->color = BLACK;
	else
	{
		Node* sibling, * parent = node->parent;
		if (parent->left == node) sibling = parent->right;
		else sibling = parent->left;

		if (!sibling || sibling->color == BLACK)
		{
			if ((!sibling->left || sibling->left->color == BLACK) && (!sibling->right || sibling->right->color == BLACK))
			{ // rodjak je crn cvor i oba njegova deteta su crni cvorovi
				if (parent->color == BLACK) parent->color = DOUBLE_BLACK;
				else parent->color = BLACK;
				sibling->color = RED;
				node->color = BLACK;
				if (parent->color == DOUBLE_BLACK) resolve_DB(t, parent);
			}
			else
			{
				Node* siblings_far_child, * siblings_near_child;
				if (node->parent->left == node) { siblings_near_child = sibling->left; siblings_far_child = sibling->right; }
				else { siblings_near_child = sibling->right; siblings_far_child = sibling->left; }

				if ((!siblings_far_child || siblings_far_child->color == BLACK) && siblings_near_child->color == RED)
				{ // rodjak je crn i njegovo dalje dete je crno, a blize crveno
					sibling->color = RED;
					siblings_near_child->color = BLACK;
					if (sibling->parent->left == sibling) left_rotation(sibling, &t->root);
					else right_rotation(sibling, &t->root);
					resolve_DB(t, node);
				}
				else if (siblings_far_child->color == RED)
				{ // rodjak je crn cvor, a njegovo dalje dete je crven cvor
					sibling->color = parent->color;
					parent->color = BLACK;
					if (node->parent->left == node) left_rotation(parent, &t->root);
					else right_rotation(parent, &t->root);
					node->color = BLACK;
					siblings_far_child->color = BLACK;
				}
			}
		}
		else if (sibling->color == RED)
		{
			sibling->color = parent->color;
			parent->color = RED;
			if (node->parent->left == node) left_rotation(parent, &t->root);
			else right_rotation(parent, &t->root);
			resolve_DB(t, node);
		}
	}
}

int RBT_delete(RBTree *t, Node* node)
{
	if (!t || !node) return -1;

	if (node->color == RED) { removeLeaf(t, node); return 0; }
	else if (node->color == BLACK) node->color = DOUBLE_BLACK;

	if (node->color == DOUBLE_BLACK)
	{
		if (t->root == node) { node->color = BLACK; removeLeaf(t, node); return 0; }
		else
		{
			Node* sibling, *parent = node->parent;
			if (parent->left == node) sibling = parent->right;
			else sibling = parent->left;

			if (!sibling || sibling->color == BLACK)
			{
				if ((!sibling->left || sibling->left->color == BLACK) && (!sibling->right || sibling->right->color == BLACK))
				{ // rodjak je crn cvor i oba njegova deteta su crni cvorovi
					if (parent->color == BLACK) parent->color = DOUBLE_BLACK;
					else parent->color = BLACK;
					sibling->color = RED;
					node->color = BLACK;
					removeLeaf(t, node);
					if (parent->color == DOUBLE_BLACK) resolve_DB(t, parent);
				}
				else
				{
					Node* siblings_far_child, * siblings_near_child;
					if (node->parent->left == node) { siblings_near_child = sibling->left; siblings_far_child = sibling->right; }
					else { siblings_near_child = sibling->right; siblings_far_child = sibling->left; }

					if ((!siblings_far_child || siblings_far_child->color == BLACK) && siblings_near_child->color == RED)
					{ // rodjak je crn i njegovo dalje dete je crno, a blize crveno
						sibling->color = RED;
						siblings_near_child->color = BLACK;
						if (sibling->parent->left == sibling) left_rotation(sibling, &t->root);
						else right_rotation(sibling, &t->root);
						RBT_delete(t, node);
					}
					else if (siblings_far_child->color == RED)
					{ // rodjak je crn cvor, a njegovo dalje dete je crven cvor
						sibling->color = parent->color;
						parent->color = BLACK;
						if (node->parent->left == node) left_rotation(parent, &t->root);
						else right_rotation(parent, &t->root);
						node->color = BLACK;
						siblings_far_child->color = BLACK;
						removeLeaf(t, node);
					}
				}
			}
			else if (sibling->color == RED)
			{
				sibling->color = parent->color;
				parent->color = RED;
				if (node->parent->left == node) left_rotation(parent, &t->root);
				else right_rotation(parent, &t->root);
				RBT_delete(t, node);
			}
		}
	}

	return 0;
}

int removeNode(RBTree *tree, Node *node)
{
	if (!tree || !node) return -1;

	//trazenje elementa za brisanje; Primenjuje se obicno BST brisanje;
	Node* toRemove = node;

	while (toRemove->left != NULL || toRemove->right != NULL)
	{ // vrtimo petlju dok ne dodjemo do lista
		
		if (toRemove->left && toRemove->right)
		{
			// cvor ima 2 deteta
			swap(toRemove, successor(toRemove)); // menjamo sa sledbenikom
			toRemove = successor(toRemove);
		}
		else if(toRemove->left) { swap(toRemove, toRemove->left); toRemove = toRemove->left; } // cvor ima jedno dete	
		else { swap(toRemove, toRemove->right); toRemove = toRemove->right; } // cvor ima jedno dete	
	}
	
	if(!RBT_delete(tree, toRemove)) tree->size--;

	return 0;
}

Node removeMin(RBTree* tree)
{
	Node dummyNode; 
	dummyNode.color = 0;
	if (!tree) return dummyNode;
	Node* node = getMin(tree);
	Node toRet = *node;
	int ret = removeNode(tree, node);
	return ret != -1 ? toRet : dummyNode;
}

Node removeMax(RBTree* tree)
{
	Node dummyNode;
	dummyNode.color = 0;
	if (!tree) return dummyNode;
	Node* node = getMax(tree);
	Node toRet = *node;
	int ret = removeNode(tree, node);
	return ret != -1 ? toRet : dummyNode;
}

int isNodeValid(Node node)
{
	if (node.color == 0) return 0;
	return 1; // valid == 1
}