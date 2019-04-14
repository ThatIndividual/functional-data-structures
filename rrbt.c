#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define BRANCH_FACTOR 8
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

struct Branch
{
    int length;
    int size_table[BRANCH_FACTOR];
    void *slots[BRANCH_FACTOR];
};

Branch *BranchNew(void);
bool  BranchPush(Branch *branch, int height, int value);

struct Leaf
{
    int length;
    int *slots;
};

Leaf *LeafNew(void);
bool  LeafPush(Leaf *leaf, int value);  

/* UTIL */

int
shift_index(int index, int shift_by)
{
    return (index >> (BRANCH_FACTOR * shift_by)) & SHIFT_MASK;
}

void *
NodeNew(int height)
{
    return height ? (void *)BranchNew() : (void *)LeafNew();
}

bool
NodePush(void *child, int height, int value)
{
    return height ? BranchPush(child, height, value) : LeafPush(child, value);
}

/* LEAF */

Leaf *
LeafNew(void)
{
    Leaf *leaf;

    leaf = malloc(sizeof(Leaf));
    leaf->length = 0;
    leaf->slots = malloc(sizeof(int) * BRANCH_FACTOR);

    return leaf;
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

int
main()
{
    Tree *t;

    t = TreeNew();
    TreePush(t, 2);
    TreePush(t, 3);
    TreePush(t, 5);
    TreePush(t, 7);

    TreePush(t, 11);
    TreePush(t, 13);
    TreePush(t, 17);
    TreePush(t, 19);

    TreePush(t, 23);
    TreePush(t, 29);
    TreePush(t, 31);
    TreePush(t, 37);

    TreePush(t, 41);
    TreePush(t, 43);
    TreePush(t, 47);
    TreePush(t, 53);

    TreePush(t, 59);
    TreePush(t, 61);
    TreePush(t, 67);
    TreePush(t, 71);
    TreePrint(t);
}

