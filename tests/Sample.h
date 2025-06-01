// 数据样本

struct P0 {};

struct P1 {
    P1() = default;
    P1(int b): a(b) {}
    int a;  // 4字节
};

struct P2 {
    int a[5];  // 20字节
};

struct P3 {
    int a[10];  // 40字节
};

struct P4 {
    int a[20];  // 80字节
};
struct P5 {
    int a[128];  // 512字节是上限
};
struct P6 {
    int a[129];  // 超宇512字节的内存池上限
};