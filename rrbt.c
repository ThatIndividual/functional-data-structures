#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define BRANCH_FACTOR 2
#define SHIFT_MASK 0b11

typedef struct Tree Tree;
typedef struct Node Node;

struct Tree
{
    int length;
    int height;
    void *root;
};

struct Node
{
    int length;
    int size_table[BRANCH_FACTOR];
    void *slots[BRANCH_FACTOR];
};

/* UTIL */

int
shift_index(int index, int shift_by)
{
    return (index >> (BRANCH_FACTOR * shift_by)) & SHIFT_MASK;
}

/* LEAF */

int *
LeafNew(void)
{
    return malloc(sizeof(int) * BRANCH_FACTOR);
}

int
LeafGet(int *leaf, int index)
{
    return leaf[shift_index(index, 0)];
}

void
LeafSet(int *leaf, int index, int value)
{
    leaf[index] = value;
}

/* NODE */

Node *
NodeNew(void)
{
    return calloc(1, sizeof(Node));
}

bool
NodePush(Node *node, int height, int value)
{
    int last_slot;
    int last_slot_length;

    last_slot = node->length - 1;
    if (last_slot == 0)
        last_slot_length = node->size_table[last_slot];
    else
        last_slot_length = node->size_table[last_slot] -
                           node->size_table[last_slot - 1];

    if (height == 1)
    {   /* Children are leaf nodes */
        if (node->length != 0 && last_slot_length != BRANCH_FACTOR)
        {   /* Can push in last slot */
            LeafSet(node->slots[last_slot], last_slot_length, value);

            node->size_table[last_slot]++;
        }
        else if (node->length != BRANCH_FACTOR)
        {   /* Can allocate new slot and push there */
            node->slots[node->length] = LeafNew();
            LeafSet(node->slots[node->length], 0, value);

            node->size_table[node->length] = node->size_table[last_slot] + 1;
            node->length++;
        }
        else /* Value cannot be pushed in the children of this node */
            return false;
    }
    else
    {   /* Children are branch nodes */
        if
        (
            node->length != 0 &&
            NodePush(node->slots[last_slot], height - 1, value)
        )   /* Could push to insert in last slot */
            node->size_table[last_slot]++;
        else if (node->length != BRANCH_FACTOR)
        {   /* Can allocate new slot and push there */
            node->slots[node->length] = NodeNew();
            NodePush(node->slots[node->length], height - 1, value);

            node->size_table[node->length] = node->size_table[last_slot] + 1;
            node->length++;
        }
        else /* Value cannot be pushed in the children of this node */
            return false;
    }

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
    Node *node;

    node = NodeNew();
    node->length = 1;
    node->size_table[0] = tree->length;
    node->slots[0] = tree->root;

    tree->height++;
    tree->root = node;
}

void
TreePush(Tree *tree, int value)
{
    if (tree->height == 0)
    {   /* Root is a leaf node */
        if (tree->length != BRANCH_FACTOR)
        {
            if (tree->length == 0)
                tree->root = LeafNew();

            LeafSet(tree->root, tree->length, value);
        }
        else
        {   /* Not enough space in the root leaf, allocate new branch */
            TreeHeighten(tree);
            NodePush(tree->root, tree->height, value);
        }
    }
    else
    {
        if (!NodePush(tree->root, tree->height, value))
        {   /* Could not push value in current root node, heighten tree */
            TreeHeighten(tree);
            NodePush(tree->root, tree->height, value);
        }
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
NodePrint(Node *node, int height, int indent)
{
    int i;

    printf("[ length: %i\n", node->length);

    print_indent(indent);
    printf(", size_table: ");
    ArrPrint(node->size_table, node->length);

    print_indent(indent);
    printf(", slots -> ");

    if (node->length == 0)
        printf("[ ]\n");
    else
    {
        for (i = 0; i < node->length - 1; i++)
        {
            if (height == 1)
            {
                int leaf_length;

                if (i == 0)
                    leaf_length = node->size_table[0];
                else
                    leaf_length = node->size_table[i] - node->size_table[i - 1];
                ArrPrint(node->slots[i], leaf_length);
            }
            else
                NodePrint(node->slots[i], height - 1, indent + 11);
            print_indent(indent + 11); 
        }
        if (height == 1)
        {
            int leaf_length;

            if (i == 0)
                leaf_length = node->size_table[0];
            else
                leaf_length = node->size_table[i] - node->size_table[i - 1];
            ArrPrint(node->slots[i], leaf_length);
        }
        else
            NodePrint(node->slots[i], height - 1, indent + 11);
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
        ArrPrint(tree->root, tree->length);
    else
        NodePrint(tree->root, tree->height, 10);

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

