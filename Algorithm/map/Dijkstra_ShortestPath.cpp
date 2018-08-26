//
//  Dijkstra_ShortestPath.cpp
//  SimpleTest
//
//  Created by 李启林 on 2018/8/26.
//  Copyright © 2018年 liqilin. All rights reserved.
//

#include "Dijkstra_ShortestPath.hpp"
#include <climits>

int vexNum = 5;
int graph[5][5] = {INT_MAX,      4,      1,INT_MAX,INT_MAX,
                         5,INT_MAX,      2,      6,      3,
                         1,      2,INT_MAX,      7,INT_MAX,
                   INT_MAX,      6,      7,INT_MAX,      2,
                   INT_MAX,      3,INT_MAX,      2,INT_MAX};

void dijkstra(int v0, int distance[vexNum], int path[vexNum]) {
    int find[vexNum];

    for (int i = 0; i < vexNum; i++) {
        distance[i] = graph[v0][i];
        path[i] = v0;
        find[i] = false;
    }
    find[v0] = true;

    for (int i = 1; i < vexNum; i++) {
        int min = INT_MAX;
        int index = 0;
        for (int k = 0; k < vexNum; k++) {
            if (distance[k] < min && !find[k]) {// !find[k]不能少，每次都是在剩余集合里面找最少的
                index = k;
                min = distance[k];
            }
        }
        
        find[index] = true;
        printf("find min: %d index: %d\n", min, index);

        for (int k = 0; k < vexNum; k++) {
            if (!find[k] && graph[index][k] != INT_MAX && min + graph[index][k] < distance[k]) {
                distance[k] = min + graph[index][k];
                path[k] = index;
                printf("set k: %d now: %d\n", k, distance[k]);
            }
        }
    }
}

void test_dijkstra() {
    int v0 = 0;
    int distance[vexNum];
    int path[vexNum];// 记录到达下标顶点最短路径的上一个顶点：path[2] = 1 代表到达顶点2的最短路径的最后一个顶点是1
    
    dijkstra(v0, distance, path);
    
    for (int i = 0; i < vexNum; i++) {
        printf("%d ", distance[i]);
        printf("%d", path[i]);
        printf("\n");
    }
}
