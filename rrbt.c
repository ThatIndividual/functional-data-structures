#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#define BRANCH_FACTOR 4
#define SHIFT_BITS 2
#define SHIFT_MASK 0b11

typedef struct Tree Tree;
typedef struct Branch Branch;
typedef struct Leaf Leaf;

struct Tree
{
    int length;
    int height;
    void *root;
};

Tree *TreeNew(void);
void  TreeHeighten(Tree *tree);
void  TreePush(Tree *tree, int value);
 int  TreeGet(Tree *tree, int index);
void  TreeSet(Tree *tree, int index, int value);

struct Branch
{
    int length;
    int size_table[BRANCH_FACTOR];
    void *slots[BRANCH_FACTOR];
};

Branch *BranchNew(void);
  bool  BranchPush(Branch *branch, int height, int value);
   int  BranchGet(Branch *branch, int height, int index);
  void  BranchSet(Branch *branch, int height, int index, int value);

struct Leaf
{
    int length;
    int slots[BRANCH_FACTOR];
};

Leaf *LeafNew(void);
bool  LeafPush(Leaf *leaf, int value);  
 int  LeafGet(Leaf *leaf, int index);
void  LeafSet(Leaf *leaf, int index, int value);

/* UTIL */

int
shift_index(int index, int shift_by)
{
    return (index >> (SHIFT_BITS * shift_by)) & SHIFT_MASK;
}

void *
NodeNew(int height)
{
    return height ? (void *)BranchNew() : (void *)LeafNew();
}

bool
NodePush(void *node, int height, int value)
{
    return height ? BranchPush(node, height, value) : LeafPush(node, value);
}

int
NodeGet(void *node, int height, int index)
{
    return height ? BranchGet(node, height, index) : LeafGet(node, index);
}

void NodeSet(void *node, int height, int index, int value)
{
    return height ? BranchSet(node, height, index, value) :
                    LeafSet(node, index, value);
}
/* LEAF */

Leaf *
LeafNew(void)
{
    return calloc(1, sizeof(Leaf));
}

bool
LeafPush(Leaf *leaf, int value)
{
    int len = leaf->length;

    if (leaf->length != BRANCH_FACTOR)
    {
        leaf->slots[leaf->length] = value;
        leaf->length++;
        return true;
    }
    else /* No space left in leaf */
        return false;
}

int
LeafGet(Leaf *leaf, int index)
{
    return leaf->slots[index];
}

void
LeafSet(Leaf *leaf, int index, int value)
{
    leaf->slots[index] = value;
}

/* NODE */

Branch *
BranchNew(void)
{
    return calloc(1, sizeof(Branch));
}

bool
BranchPush(Branch *branch, int height, int value)
{
    int last_slot;

    last_slot = branch->length - 1;
    if
    (
        branch->length != 0 &&
        NodePush(branch->slots[last_slot], height - 1, value)
    )   /* Could push in last slot */
        branch->size_table[last_slot]++;
    else if (branch->length != BRANCH_FACTOR)
    {   /* Can allocate new slot and push there */
        branch->slots[branch->length] = NodeNew(height - 1);
        NodePush(branch->slots[branch->length], height - 1, value);

        branch->size_table[branch->length] = branch->size_table[last_slot] + 1;
        branch->length++;
    }
    else /* Value cannot be pushed in the children of this branch */
        return false;

    return true;
}

int
BranchGet(Branch *branch, int height, int index)
{
    int shifted_index;

    shifted_index = shift_index(index, height);
    /* Check the index and adjust if neccesary */
    while (index > branch->size_table[shifted_index])
        shifted_index++;

    if (shifted_index == 0)
        return NodeGet(branch->slots[0], height - 1, index);
    else
        return NodeGet(branch->slots[shifted_index],
                       height - 1,
                       index - branch->size_table[shifted_index - 1]);
}

void
BranchSet(Branch *branch, int height, int index, int value)
{
    int shifted_index;

    shifted_index = shift_index(index, height);
    /* Check the index and adjust if neccesary */
    while (index > branch->size_table[shifted_index])
        shifted_index++;

    if (shifted_index == 0)
        NodeSet(branch->slots[0], height - 1, index, value);
    else
        NodeSet(branch->slots[shifted_index],
                height - 1,
                index - branch->size_table[shifted_index - 1],
                value);
}

/* TREE */

Tree *
TreeNew(void)
{
    return calloc(1, sizeof(Tree));
}

void
TreeHeighten(Tree *tree)
{
    Branch *branch;

    branch = BranchNew();
    branch->length = 1;
    branch->size_table[0] = tree->length;
    branch->slots[0] = tree->root;

    tree->height++;
    tree->root = branch;
}

void
TreePush(Tree *tree, int value)
{
    if (tree->length == 0)
        tree->root = LeafNew();
    if (!NodePush(tree->root, tree->height, value))
    {   /* Could not push value in current root node, heighten tree */
        TreeHeighten(tree);
        NodePush(tree->root, tree->height, value);
    }

    tree->length++;
}

int
TreeGet(Tree *tree, int index)
{
    assert(index < tree->length);
    return NodeGet(tree->root, tree->height, index);
}

void
TreeSet(Tree *tree, int index, int value)
{
    assert(index < tree->length);
    return NodeSet(tree->root, tree->height, index, value);
}

void
TreePushArray(Tree *tree, int arr_len, int *arr)
{
    int i;

    for (i = 0; i < arr_len; i++)
        TreePush(tree, arr[i]);
}

/* MISC */

void
print_indent(int indent)
{
    int i;

    for (i = 0; i < indent; i++)
        putchar(' ');
}

void
ArrPrint(int *arr, int length)
{
    int i;

    if (length == 0)
        printf("[ ]\n");
    else
    {
        printf("[ ");
        for (i = 0; i < length - 1; i++)
            printf("%i, ", arr[i]);
        printf("%i ]\n", arr[i]);
    }
}

void
LeafPrint(Leaf *leaf)
{
    ArrPrint(leaf->slots, leaf->length);
}

void
BranchPrint(Branch *branch, int height, int indent)
{
    int i;

    printf("[ length: %i\n", branch->length);

    print_indent(indent);
    printf(", size_table: ");
    ArrPrint(branch->size_table, branch->length);

    print_indent(indent);
    printf(", slots -> ");

    if (branch->length == 0)
        printf("[ ]\n");
    else
    {
        for (i = 0; i < branch->length - 1; i++)
        {
            if (height == 1)
                LeafPrint(branch->slots[i]);
            else
                BranchPrint(branch->slots[i], height - 1, indent + 11);
            print_indent(indent + 11); 
        }
        if (height == 1)
            LeafPrint(branch->slots[i]);
        else
            BranchPrint(branch->slots[i], height - 1, indent + 11);
        print_indent(indent);
        printf("]\n");
    }
}

void
TreePrint(Tree *tree)
{
    printf("[ height: %i\n", tree->height);
    printf(", length: %i\n", tree->length);
    printf(", root -> ");

    if (tree->height == 0)
        LeafPrint(tree->root);
    else
        BranchPrint(tree->root, tree->height, 10);

    printf("]\n");
}

#undef BRANCH_FACTOR

int primes[] = {  2,   3,   5,   7,  11,  13,  17,  19,  23,  29,
                 31,  37,  41,  43,  47,  53,  59,  61,  67,  71,
                 73,  79,  83,  89,  97, 101, 103, 107, 109, 113,
                127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
                179, 181, 191, 193, 197, 199, 211, 223, 227, 229,
                233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
                283, 293, 307, 311, 313, 317, 331, 337, 347, 349,
                353, 359, 367, 373, 379, 383, 389, 397, 401, 409,
                419, 421, 431, 433, 439, 443, 449, 457, 461, 463,
                467, 479, 487, 491, 499, 503, 509, 521, 523, 541};

int
main()
{
    Tree *t;

    t = TreeNew();
    TreePushArray(t, 100, primes);
    printf("77: %i\n", TreeGet(t, 77));

    TreeSet(t, 77, 77);
    printf("77: %i\n", TreeGet(t, 77));
}

