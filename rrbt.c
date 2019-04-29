#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#define BRANCH_FACTOR 4
#define SHIFT_BITS 2
#define SHIFT_MASK 0b11
#define AVG_COMPACT 1

typedef struct Tree Tree;
typedef struct Branch Branch;
typedef struct Leaf Leaf;
typedef struct branch_pair branch_pair;

struct branch_pair
{
    Branch *left;
    Branch *right;
};

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
Tree *TreeConcat(Tree *left, Tree *right);

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
  bool  BranchPushNode(Branch *parent, void *child, int child_len);

branch_pair BranchHighConcat(Branch *left, Branch *right);
branch_pair BranchLowConcat(Branch *left, Branch *right);

struct Leaf
{
    int length;
    int slots[BRANCH_FACTOR];
};

Leaf *LeafNew(void);
bool  LeafPush(Leaf *leaf, int value);  
 int  LeafGet(Leaf *leaf, int index);
void  LeafSet(Leaf *leaf, int index, int value);
void  LeafPushArray(Leaf *leaf, int arr_len, int *arr);

/* UTIL */

int
shift_index(int index, int shift_by)
{
    return (index >> (SHIFT_BITS * shift_by)) & SHIFT_MASK;
}

/*
Here `compactness' is defined as the number of steps we have to take (on
average) after the initial index shift to find the slot containig the index
we're looking for.

A tree with a compactness of 0 would degenerate into a non-relaxed radix
balanced trie, and in order to maintain it, we would have to always compact.

A tree with a compactness of 2 on the other hand, would trigger compacting less
often, but on average we have to do a 2 step linear search to find the slot we
are looking for.
*/

int
compactness(int nodes, int slots)
{
    return nodes - ((slots - 1) / BRANCH_FACTOR) - 1;
}

/*
Given two arrays of pointers to leafs, read from `src' and push merged leafs on
`dst'. For exemple, given the following `src', this is how an initially empty
`dst' will look after running this function:

            ┌──┬──┐ ┌──┬──┬──┐ ┌──┬──┐         ┌──┬──┬──┬──┐ ┌──┬──┬──┐
        src │ 5│ 6│ │ 7│ 8│ 9│ │10│11│     dst │ 5│ 6│ 7│ 8│ │ 9│10│11│
            └──┴──┘ └──┴──┴──┘ └──┴──┘         └──┴──┴──┴──┘ └──┴──┴──┘

The amount of leafs to merge is specified by `length'. Note that a buffer
overrun will happen if `dst' does not have enough space to store all the
resulting leafs.
*/

void
squash_leafs(Leaf **src, Leaf **dst, int length)
{
    int node_i, // index into the `src' leaf array
        slot_i; // index into the curent leaf's slot array
    Leaf *leaf; // leaf in which the item of the loose leafs are squashed

    if (length == 0)
        return;

    leaf = LeafNew();
    for (node_i = 0; node_i < length; node_i++)
    {
        Leaf *curr_leaf;
       
        curr_leaf = src[node_i];
        for (slot_i = 0; slot_i < curr_leaf->length; slot_i++)
        {
            bool pushed;   // managed to push a value into the squashing leaf
            int curr_item; 

            curr_item = curr_leaf->slots[slot_i];
            pushed = LeafPush(leaf, curr_item);
            if (!pushed)
            {
                *dst++ = leaf;
                leaf = LeafNew();
                LeafPush(leaf, curr_item);
            }
        }
    }

    if (leaf->length)
        *dst = leaf;
}

void
squash_branches(Branch **src, Branch **dst, int length)
{
    int node_i,
        slot_i;
    Branch *branch;

    if (length == 0)
        return;

    branch = BranchNew();
    for (node_i = 0; node_i < length; node_i++)
    {
        Branch *curr_branch;

        curr_branch = src[node_i];
        for (slot_i = 0; slot_i < curr_branch->length; slot_i++)
        {
            bool pushed;
            int curr_slot_len;
            void *curr_slot;

            if (slot_i)
                curr_slot_len = curr_branch->size_table[slot_i] -
                                curr_branch->size_table[slot_i - 1];
            else
                curr_slot_len = curr_branch->size_table[slot_i];

            curr_slot = curr_branch->slots[slot_i];
            pushed = BranchPushNode(branch, curr_slot, curr_slot_len);
            if (!pushed)
            {
                *dst++ = branch;
                branch = BranchNew();
                BranchPushNode(branch, curr_slot, curr_slot_len);
            }
        }
    }

    if (branch-length)
        *dst = branch;
}

/*
Given an array of leafs, merge them until we are left with `to_remove' fewer
nodes. For example, passing the following `src' with a `to_remove' of 1, would
return the following `ret' leaf array.

         0┌──┬──┬──┬──┐1┌──┬──┐2┌──┬──┬──┐3┌──┬──┐4┌──┬──┐5┌──┬──┬──┐6
     src  │ 1│ 2│ 3│ 4│ │ 5│ 6│ │ 7│ 8│ 9│ │10│11│ │12│13│ │14│15│16│
          └──┴──┴──┴──┘ └──┴──┘ └──┴──┴──┘ └──┴──┘ └──┴──┘ └──┴──┴──┘
          0┌──┬──┬──┬──┐1┌──┬──┬──┬──┐2┌──┬──┬──┐3┌──┬──┐4┌──┬──┬──┐5
      ret  │ 1│ 2│ 3│ 4│ │ 5│ 6│ 7│ 8│ │ 9│10│11│ │12│13│ │14│15│16│
           └──┴──┴──┴──┘ └──┴──┴──┴──┘ └──┴──┴──┘ └──┴──┘ └──┴──┴──┘

After we have reached the first node with free slots. Starting from this index,
we probe forwards until we have selected a run of nodes that, once squashed
together, would leave us with `to_remove' fewer nodes.  Meaning:

XXX formnulas wrong XXX

⌊selected slots / branching factor⌋ ≤ selected nodes - merge amount

Continuing with the above example:

1┌──┬──┐2┌──┬──┬──┐3            for a run of 2:
 │ 5│ 6│ │ 7│ 8│ 9│             ⌊5 / 4⌋ ≰ 2 - 1
 └──┴──┘ └──┴──┴──┘   
1┌──┬──┐2┌──┬──┬──┐3┌──┬──┐4    for a run of 3:
 │ 5│ 6│ │ 7│ 8│ 9│ │10│11│     ⌊7 / 4⌋ ≤ 3 - 1
 └──┴──┘ └──┴──┴──┘ └──┴──┘

Thus, we can satisfy a `to_remove' constraint of 1 by squashing leafs indexes
1 to 4.
*/

Leaf **
merge_leafs(Leaf **src, int src_len, int to_remove)
{
    int src_i,
        ret_i;
    Leaf **ret;

    if (to_remove == 0)
        return src;

    src_i = ret_i = 0;
    ret = malloc(sizeof(Leaf *) * (src_len - to_remove));

    while (src[src_i]->length == BRANCH_FACTOR)
        ret[ret_i++] = src[src_i++];

    {
        int selected_nodes,
            selected_slots,
            squashed_nodes;

        selected_nodes = 2;
        selected_slots = src[src_i]->length;
        while (true)
        {
            assert(selected_nodes + src_i <= src_len);

            selected_slots += src[src_i + selected_nodes - 1]->length;
            squashed_nodes = ((selected_slots - 1) / BRANCH_FACTOR) + 1;
            if (squashed_nodes <= selected_nodes - to_remove)
            {
                squash_leafs(src + src_i, ret + src_i, selected_nodes);
                src_i += selected_nodes;
                ret_i += squashed_nodes;
                break;
            }
            else
                selected_nodes++;
        }
    }

    while (src_i < src_len)
        ret[ret_i++] = src[src_i++];

    return ret;
}

Branch **
merge_branches(Branch **src, int src_len, int to_remove)
{
    int src_i,
        ret_i;
    Branch **ret;

    if (to_remove == 0)
        return src;

    src_i = ret_i = 0;
    ret = malloc(sizeof(Branch *) * (src_len - to_remove));

    while (src[src_i]->length == BRANCH_FACTOR)
        ret[ret_i++] = src[src_i++];

    {
        int selected_nodes,
            selected_slots,
            squashed_nodes;

        selected_nodes = 2;
        selected_slots = src[src_i]->length;
        while (true)
        {
            assert(selected_nodes + src_i <= src_len);

            selected_slots += src[src_i + selected_nodes - 1]->length;
            squashed_nodes = ((selected_slots - 1) / BRANCH_FACTOR) + 1;
            if (squashed_nodes <= selected_nodes - to_remove)
            {
                squash_branches(src + src_i, ret + src_i, selected_nodes);
                src_i += selected_nodes;
                ret_i += squashed_nodes;
                break;
            }
            else
                selected_nodes++;
        }
    }

    while (src_i < src_len)
        ret[ret_i++] = src[src_i++];

    return ret;
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

void
LeafPushArray(Leaf *leaf, int arr_len, int *arr)
{
    int i;

    for (i = 0; i < arr_len; i++)
        LeafPush(leaf, arr[i]);
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

bool
BranchPushNode(Branch *parent, void *child, int child_len)
{
    if (parent->length == BRANCH_FACTOR)
        return false;

    parent->slots[parent->length] = child;
    if (parent->length == 0)
        parent->size_table[0] = child_len;
    else
        parent->size_table[parent->length] =
                parent->size_table[parent->length - 1] + child_len;
    parent->length++;

    return true;
}

branch_pair
BranchLowConcat(Branch *left, Branch *right)
{
    int num_nodes,
        num_slots,
        to_remove,
        left_len,
        right_len,
        i;
    Leaf **leafs,
         **curr_leaf,
         **merged_leafs;
    branch_pair ret;

    /* marshall nodes to array */
    num_nodes = left->length + right->length;
    leafs = malloc(sizeof(Leaf *) * num_nodes);
    curr_leaf = leafs;

    for (i = 0; i < left->length; i++)
        *curr_leaf++ = left->slots[i];
    for (i = 0; i < right->length; i++)
        *curr_leaf++ = right->slots[i];

    num_slots = 0;
    for (i = 0; i < num_nodes; i++)
        num_slots += leafs[i]->length;

    to_remove = compactness(num_nodes, num_slots) - AVG_COMPACT;
    if (to_remove == 0)
    {   /* the branches do not require compacting */
        ret.left = left;
        ret.right = right;
        free(leafs);
        return ret;
    }
    merged_leafs = merge_leafs(leafs, num_nodes, to_remove);

    /* unmarshall leafs to branch pair */
    num_nodes -= to_remove;
    right_len = num_nodes > BRANCH_FACTOR ? num_nodes - BRANCH_FACTOR : 0;
    left_len = num_nodes - right_len;
    if (left_len)
    {
        ret.left = BranchNew();
        for (i = 0; i < left_len; i++)
            BranchPushNode(ret.left, merged_leafs[i], merged_leafs[i]->length);

        if (right_len)
        {
            ret.right = BranchNew();
            for (i = 0; i < right_len; i++)
                BranchPushNode(ret.right,
                               merged_leafs[left_len + i],
                               merged_leafs[left_len + i]->length);
        }
        else
            ret.right = NULL;
    }
    else
        ret.left = ret.right = NULL;

    free(leafs);
    return ret;
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

Tree *
TreeConcat(Tree *left, Tree *right)
{
    branch_pair result;

    assert(left->height == 1);
    assert(right->height == 1);

    result = BranchLowConcat(left->root, right->root);
    if (result.left)
    {
        Tree *new_tree;
        
        new_tree = TreeNew();
        if (result.right)
        {   /* Both resulting branches contain nodes */
            Branch *new_root;

            new_root = BranchNew();
            BranchPushNode(new_root, result.left, result.left->length);
            BranchPushNode(new_root, result.right, result.right->length);

            new_tree->length = new_root->length;
            new_tree->height = 2;
            new_tree->root = new_root;

            return new_tree;
        }
        else
        {   /* All the values are in the left result node */
            new_tree->length = result.left->length;
            new_tree->height = 1;
            new_tree->root = result.left;

            return new_tree;
        }
    }
    else /* The two trees are empty, return an empty tree */
        return left;
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
#undef SHIFT_BITS
#undef SHIFT_MASK
#undef AVG_COMPACT

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
    Tree *tree_1, *tree_2, *tree_result;
    Branch *branch_1, *branch_2;
    Leaf *leaf_1, *leaf_2, *leaf_3, *leaf_4, *leaf_5, *leaf_6;

    branch_1 = BranchNew();
    leaf_1 = LeafNew(); LeafPushArray(leaf_1, 4, (int[]){ 1,  2,  3,  4});
    leaf_2 = LeafNew(); LeafPushArray(leaf_2, 2, (int[]){ 5,  6});
    BranchPushNode(branch_1, leaf_1, leaf_1->length);
    BranchPushNode(branch_1, leaf_2, leaf_2->length);

    branch_2 = BranchNew();
    leaf_3 = LeafNew(); LeafPushArray(leaf_3, 3, (int[]){ 7,  8, 9});
    leaf_4 = LeafNew(); LeafPushArray(leaf_4, 2, (int[]){10, 11});
    leaf_5 = LeafNew(); LeafPushArray(leaf_5, 2, (int[]){12, 13});
    leaf_6 = LeafNew(); LeafPushArray(leaf_6, 3, (int[]){14, 15, 16});
    BranchPushNode(branch_2, leaf_3, leaf_3->length);
    BranchPushNode(branch_2, leaf_4, leaf_4->length);
    BranchPushNode(branch_2, leaf_5, leaf_5->length);
    BranchPushNode(branch_2, leaf_6, leaf_6->length);

    tree_1 = TreeNew();
    tree_1->length = 6;
    tree_1->height = 1;
    tree_1->root = branch_1;
    TreePrint(tree_1);
    printf("\n\n");

    tree_2 = TreeNew();
    tree_2->length = 10;
    tree_2->height = 1;
    tree_2->root = branch_2;
    TreePrint(tree_2);
    printf("\n\n");

    tree_result = TreeConcat(tree_1, tree_2);
    TreePrint(tree_result);
}

