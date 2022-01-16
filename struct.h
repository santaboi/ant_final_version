#include <stdlib.h>
#include <stdio.h>

// for mpi use
struct
{
    int cost;
    int rank;
} global_data, loc_data;

typedef struct city
{ // for read in file and tokenize it, then put into a node
    char *c_name;
    struct city *next;
} city;

typedef struct Queue
{
    city *front_p;
    city *end_p;
} Queue;

void enqueue(Queue *q, char *token)
{
    city *temp = malloc(sizeof(city));
    temp->next = NULL;
    temp->c_name = malloc(strlen(token) + 1);
    memcpy(temp->c_name, token, strlen(token));

    if (!q->front_p)
        q->front_p = q->end_p = temp;
    else
    {
        q->end_p->next = temp;
        q->end_p = temp;
    }
}
city *dequeue(Queue *q)
{
    city *temp;
    if (!q->front_p)
        temp = NULL;
    else if (q->front_p == q->end_p)
    {
        temp = q->front_p;
        q->front_p = q->end_p = NULL;
    }
    else
    {
        temp = q->front_p;
        q->front_p = q->front_p->next;
    }
    return temp;
}