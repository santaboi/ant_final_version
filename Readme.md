# parallel programming(HW6 ant algorithm)
 
## Algorithm Comprehension
* https://www.youtube.com/watch?v=783ZtAF4j5g
Ant optimized algorithm referenced
(big thanks to the buddy) 

first step(calculate 1. 2. matrix)
shorter the path higher the pheromone it gets
* ![](https://i.imgur.com/qvcQTqo.png)
* 若沒走pheromone 即 0
![](https://i.imgur.com/NRzJc3e.png)
* small alpha for sub problems
* 1. cost matrix 2.pheromone matrix

second step(choose path)
*determine the probabilities of each path
![](https://i.imgur.com/sDZUOb5.png)
![](https://i.imgur.com/lYGpEXJ.png)
![](https://i.imgur.com/OlamDV4.png)

## Compile & Run
```
$ mpicc -fopenmp -lm -o hw6 hw6.c
$ mpiexec -n 8 ./hw6_problem1 "textfile_name"
```
## What I have done
1. 把 thread_num定義在 code 裡面可改 (不用cmd取得)
**確實有用mpi跑出8個process 跟 每個process都有8個threads**![](https://i.imgur.com/TSRp6k0.png)

2.用了HW5的觀念enqueue dequeue

3.定義pheromone_array

4.global_min local_min要注意外面比一次critical內部也比一次
+ 用mpireduce()內建的MPI_MINLOC去比

5.MPI用在main的架構去把row col切分給各個process再讓每個process(node)內部去fork出多個threads

+ **因為剛開始無法-ppn成功，故就把thread_nu macro定義在global scope(結果一樣是成功的)**

6.螞蟻內部的算法就不多贅述
+ prob_calculate()
+ path_choosing()
+ 以上兩個都在主要的ant_optimized()內被呼叫
+ 在ant_omptimized()內部
1.用 #pragma omp parallel num_threads(thread_num) 把原本的螞蟻算法平行化
2 用 #pragma omp critical 保護globla更新的部分ˇ
```c=
#pragma omp critical
                    {
                        if (best_cost > len)
                        {
                            best_cost = len;
                            memcpy(min_path, ant_array[k], col * sizeof(int));
                        }
                    }
```

## Experiment Results
+ 8 process 8 thread_num in each process
+ 基本參數設定
    ```
     thread_num 8
     iterations 100
     
     ant_num 100
     alpha 1
     beta 3
     total_sum_of_pheromone 400000.0
     pheromone_vapor_rate 0.35
    ```
+ 後面主要是動螞蟻數量跟 iteration loop
    * 動pheromone 的改變比較小 
### "ATT48" minmal tour -> 33523
1. 參數沒變 : best_cost 34230 ![](https://i.imgur.com/zblb3pH.png)
2. (ant_num = 250 , iterations = 250): best_cost 33923 ![](https://i.imgur.com/t6ThQEA.png)
3.(ant_num = 350 , iterations = 350): best_cost 33897 ![](https://i.imgur.com/gga0HS9.png)
4.(ant_num = 350 , iterations = 350 ,vapor_rate = 0.25) best_cost 
 

### "DANTZIG42" minimal tour->699
1. 參數沒變 : best_cost 704 ![](https://i.imgur.com/dg25rp1.png)
2. (ant_num = 250 , iterations = 250): best_cost 703 ![](https://i.imgur.com/2FwV0me.png)


### "FRI26" minimal tour->937
1. 參數沒變 : best_cost 937 ![](https://i.imgur.com/WzQ8kZr.png)
2. (ant_num = 250 , iterations = 250): best_cost 937 ![](https://i.imgur.com/yJ0zZAj.png)

### "GR17" mininal tour->2085
1. 參數沒變 : best_cost (跑不出來)???
->我有嘗試debug了
->原本是以為mpi非整除部分有問題(檢查結果應該沒問題)
->我快速用相同的算法跑了一個notebook檔案
結果如下:(希望能彌補個C跑不出來)
我覺得python把平行程式包的更好用(寫起來相對方便)
![](https://i.imgur.com/96RyC1B.png)

+ **從上面可以看出把螞蟻數量加多更大迴圈數加多，可以使大問題跑得更準**

## Any difficulties (got some enormous problems)
+ gr17_d真的不知道為何跑不出來(會一直跳collective abort真的de不出來)
![](https://i.imgur.com/86GX2OA.png)

![](https://i.imgur.com/zlHWGLw.png)
->只有pn1 連不到pn2 pn4 的error
![](https://i.imgur.com/LY5wkyl.png)
->發現mpd.hosts裡面莫名其妙是全空的
->試著在內部加pn2 pn4 加完成後還是ssh pn2 pn4失敗
![](https://i.imgur.com/2nNUaXS.png)
->有常試詢問助教看看

![](https://i.imgur.com/oXQZKIS.png)
gr17_d.txt 莫名其妙跑不出來

助教這個作業在前面一兩個禮拜很多考試時，都只有在寫關於演算法的部分
1/13-1/14才開始改成parallel的形式
但好像實驗室的電腦怪怪的只有pn1 (pn2 pn4 都連線不到)，也有可能是我的server設定壞掉了嗎?我上網查自己解決不太了...
有在1/14晚上寄信給助教了，沒有cluster可以測試所以也不確定自己平行化完成的code是否正確，原本要補跑HW5的作業要補交hw5報告，也因此跑不了(非常不好意思，以下部分可能只能寫出我用的方法，結果無)


