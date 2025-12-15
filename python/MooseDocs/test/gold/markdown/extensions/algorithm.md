# Algorithm Extension {#algorithm-extension}

[Algorithm 1: Test algorithm]{#testalgo}

1: **function** testfunction(a)  
2:     **procedure** testprocedure(b)  
3:         **for** 1,N **do**  
4:             **while** True **do**  
5:                 **if** False **then**  
6:                     Statement 1  
7:                 **else if** False **then**  
8:                     Statement 2  
9:                 **else**  
10:                     Statement 3 # ▹ Comment  
11:                 **end if**  
12:             **end while**  
13:         **end for**  
14:     **end procedure**  
15: **end function**  

[Algorithm 2: The Bellman-Kalaba algorithm]{#bk}

1: **function** BellmanKalab($G$, $u$, $l$, $p$})  
2:     **for** $v\in V(G)$ **do**  
3:         $l(v) \leftarrow \infty$  
4:     **end for**  
5:     $l(u) \leftarrow 0$  
6:     **while** $changed$ **do** # ▹ Initial is
$changed \leftarrow \text{True}$  
7:         **for** $i \leftarrow 1, n$ **do**  
8:             $min \leftarrow l(v_i)$  
9:             **for** $j \leftarrow 1, n$ **do**  
10:                 **if** $min > e(v_i, v_j) + l(v_j)$ **then**  
11:                     $min \leftarrow e(v_i, v_j) + l(v_j)$  
12:                     $p(i) \leftarrow v_j$  
13:                 **end if**  
14:             **end for**  
15:             $l’(i) \leftarrow min$  
16:         **end for**  
17:         $changed \leftarrow l \not= l’$  
18:         $l \leftarrow l’$  
19:     **end while**  
20: **end function**  

$y = mx + b$