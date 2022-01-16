#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include "struct.h"
#define thread_num 4
#define iterations 100
#define ant_num 100
#define alpha 1
#define beta 3
#define total_sum_of_pheromone 400000.0
#define pheromone_vapor_rate 0.35


/*those struct thing is place at struct.h to make the sphegatti code more cleaner*/
//global var
int **distance; // the map that indicated that the distance between the node
int row, col;
int best_cost = INT_MAX; // global min cost for omp

int *min_path;          // global min path
int id, comm_sz;        // mpi id and mpi process number

//funtion to use
city *dequeue(Queue *q);
void enqueue(Queue *q, char *token);
int **dynamic_int(int row, int col);
double **dynamic_double(int row, int col);
double prob_calculate(int start , double **pheromone, int *checking_array);
void path_choosing(int row_c , double **pheromone, int **path);
void ant_optimized();
int read_file(char *files);


int main(int argc, char *argv[])
{

    int i;

    /* MPI Start*/
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    //printf("process : %d" , id);
    /* try get parameters from cmd */
    if (argc != 2)
    {
        printf("wrong total number of argv\n");
        exit(1) ;
    }

    /* process0 read in file */
    if (!id)
    {
        if (read_file(argv[1]))
            printf("read file success!!!\n");
        else{
            printf("read file fails!!\n");
            exit(1) ;
        }
    }


    MPI_Barrier(MPI_COMM_WORLD); // synchronize each process
    /* broadcast row col */
    MPI_Bcast(&row, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&col, 1, MPI_INT, 0, MPI_COMM_WORLD);

    /* global scope min path */
    min_path = malloc(col * sizeof(int));

    /* definitoin of the map of the distance if process id is not 0 */
    if (id != 0)
        distance = dynamic_int(row, col);

    /* every process get row col info */
    for (i = 0; i < row; i++)
        MPI_Bcast(distance[i], col, MPI_INT, 0, MPI_COMM_WORLD);

    double start_time, end_time;
    start_time = MPI_Wtime();
    /* the main execution loop*/
    ant_optimized();
    end_time = MPI_Wtime();
    double total_time = end_time - start_time;

    MPI_Barrier(MPI_COMM_WORLD); // synchronize each process

    /* 用reduce 去比 */
    loc_data.rank = id;
    loc_data.cost = best_cost;
    MPI_Allreduce(&loc_data, &global_data, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);

    if (global_data.rank != 0)
    { /* rank 0 has best tour */
        if (id == 0)
            MPI_Recv(min_path, col, MPI_INT, global_data.rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        else if (id == global_data.rank)
            MPI_Send(min_path, col, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
    if (!id)
    {/*rank 0*/
        printf("best_cost = %d\n", global_data.cost);
        for (i = 0; i < col; i++)
        {
            printf("%d", min_path[i]);
            if (i != col - 1)
            {
                printf("~");
            }
        }
        printf("\n");
        printf("run time = %f\n", total_time);
    }

    free(distance);
    free(distance[0]);

    MPI_Finalize();

    return 0;
}



int **dynamic_int(int row, int col)
{
    int i;
    int **temp = (int **)malloc(sizeof(int *) * row);
    int *temp2 = (int *)malloc(sizeof(int) * row * col);
    memset(temp, 0, sizeof(int) * row);
    memset(temp2, 0, sizeof(int) * row * col);

    for (i = 0; i < row; i++)
    {
        temp[i] = &temp2[i * col];
    }

    return temp;
}

double **dynamic_double(int row, int col)
{
    int i;
    double **temp = (double **)malloc(sizeof(double *) * row);
    double *temp2 = (double *)malloc(sizeof(double) * row * col);
    memset(temp, 0, sizeof(double) * row);
    memset(temp2, 0, sizeof(double) * row * col);

    for (i = 0; i < row; i++)
    {
        temp[i] = &temp2[i * col];
    }
    return temp;
}

double prob_calculate(int start , double **pheromone, int *checking_array)
{
    double probab = 0.0;
    int i = 0;
    for (; i < col; i++)
    {
        /* total probility sum calculation */
        if (checking_array[i] == 0)
        {
            probab += (double)(pow((double)1.0 / distance[start][i], beta) * pow(pheromone[start][i], alpha));
        }
    }
    return probab;
}

void path_choosing(int row_c , double **pheromone, int **path)
{
    int *checking_array = malloc(sizeof(int) * col); // the matrix for check if the ant has been gone yet
    int i, j, find;
    double prob;
    double total;
    double next;

    // init checking_array
    memset(checking_array, 0, sizeof(int) * col);
    checking_array[path[row_c][0]] = 1;

    for (i = 1; i < col; i++)
    {
        prob = prob_calculate(path[row_c][i - 1] , pheromone, checking_array);
        next = (double)rand() / (RAND_MAX + 1.0); // decide the porb num where to go
        j = -1;
        total = 0.0;
        find = 0;
        while ((double)total / prob < next && find == 0)
        {
            while ((++j < col) && checking_array[j] == 1)
            {
                ;
            }
            if (j == col)
            {
                while (checking_array[--j] == 1)
                    ;
                find = 1;
            }
            total += (double)pow(pheromone[path[row_c][i - 1]][j], alpha) * pow((double)1.0 / distance[path[row_c][i - 1]][j], beta);
        }
        path[row_c][i] = j;
        checking_array[j] = 1;
    }
    free(checking_array);
}

void ant_optimized()
{
#pragma omp parallel num_threads(thread_num)
    {   
        int my_id = omp_get_thread_num(); 
        double pheromone_per_route;
        int len;
        int i, m, n;
        /*total parallel checking*/
        //printf("process %d , thread %d \n" , id , my_id) ;

        srand(id * comm_sz + my_id * thread_num); //用mpi rank 加上 thread id 當seed

        double **pheromone = dynamic_double(row, col);
        int **ant_array = dynamic_int(ant_num, col);
        // random start point for each ants
        for (n = 0; n < ant_num; n++)
        {
            ant_array[n][0] = rand() % col;
            for (i = 1; i < col; i++)
                ant_array[n][i] = -1;
        }


        // for initial pheromone matrix
        for (i = 0; i < row; i++)
        {
            for (m = 0; m < col; m++)
                pheromone[i][m] = 1.0;
        }

        for (m = 0; m < iterations ; m++)
        {
            /* choose route for each ants*/
            for (n = 0; n < ant_num; n++)
                path_choosing(n , pheromone, ant_array);

            /* compare the distance is min or not (get each ants distance) */
            for (n = 0; n < ant_num; n++)
            {
                len = 0;
                for (i = 1; i < col; i++)
                    len += distance[ant_array[n][i - 1]][ant_array[n][i]];
                len += distance[ant_array[n][col - 1]][ant_array[n][0]];
                if (best_cost > len)
                {
#pragma omp critical
                    {
                        if (best_cost > len)
                        {
                            best_cost = len;
                            memcpy(min_path, ant_array[k], col * sizeof(int));
                        }
                    }
                }
            }

            // pheromone array update
            for (i = 0; i < row; i++)
            {
                for (n = 0; n < col; n++)
                {
                    pheromone[i][n] *= (1.0 - pheromone_vapor_rate);
                }
            }
            pheromone_per_route = total_sum_of_pheromone / (double)len;
            for (n = 0; n < ant_num; n++)
            {
                for (i = 1; i < col; i++)
                {
                    pheromone[ant_array[n][i - 1]][ant_array[n][i]] += pheromone_per_route;
                    pheromone[ant_array[n][i]][ant_array[n][i - 1]] += pheromone_per_route;
                }
            }
        }

        free(pheromone);
        free(pheromone[0]);
        free(ant_array);
        free(ant_array[0]);
    }
}

int read_file(char *files)
{
    Queue *msg = malloc(sizeof(Queue));
    msg->front_p = msg->end_p = NULL;
    char buffer[10000];
    int i = 0, j = 0;

    char *token;
    city *temp;

    FILE *fp = fopen(files, "r");
    if (!fp)
    {
        printf("cant open fails!!!!!\n");
        exit(1);
    }

    while (fgets(buffer, 9999, fp))
    {
        j = 0;
        token = strtok(buffer, " "); // get token
        while (token)
        {
            j++;
            enqueue(msg, token);
            token = strtok(NULL, " ");
        }
        i++;
    }
    i = j;
    row = i;
    col = j;

    distance = dynamic_int(row, col);

    // dequeue the node and put the token in the node to the map
    for (i = 0; i < row; i++)
    {
        for (j = 0; j < col; ++j)
        {
            temp = dequeue(msg);
            distance[i][j] = atoi(temp->c_name); // atoi change string to int
            free(temp);
        }
    }

    fclose(fp);
    free(msg);
    return 1;
}