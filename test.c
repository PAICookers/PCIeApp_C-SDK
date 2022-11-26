#include<stdio.h> //包含标准输入输出函数库

int main()
{
    int a = 256*1024;
    printf("字符型字长为%d\n",sizeof(char));

    printf("整型字长为%d\n",a+1);

    printf("长整型字长为%d\n",sizeof(long));

    printf("单精度浮点型字长为%d\n",sizeof(float));

    printf("双精度浮点型字长为%d\n",sizeof(double));

}