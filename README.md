C++常用工具
----------



channel
-----------
参照golang的channel，使用条件变量来设计的一个堵塞型队列。
- 用来做线程通信，处理异步数据 
- 还可以拿来作为有限对象池，比如连接对象，初始化固定数量，取出连接使用，还回连接待用，取完就堵塞。 
- 真实使用时需要考虑互斥锁和条件变量的性能影响
