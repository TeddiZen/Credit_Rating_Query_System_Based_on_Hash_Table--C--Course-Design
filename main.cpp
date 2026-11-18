#define _CRT_SECURE_NO_WARNINGS
// ============================================================================
//  课程设计：基于哈希表的信用等级查询系统
//
//  存储结构：链地址法（静态数组 + 单链表），按身份证号进行查询、修改、
//            删除、插入、打印和文件存取。
//
//  哈希函数：数字分析法 + 除留余数法
//      H(pid) = ((id[3]-'0')*10^4 + (id[9]-'0')*10^3 + (id[11]-'0')*10^2
//               + (id[13]-'0')*10^1 + (id[16]-'0')) % M
//      即取身份证号的第 4、10、12、14、17 位组成一个新的整数再对 M 取余。
//
//  注意：身份证号固定 18 位，而 PidType 只有 18 个字节，装不下 '\0'。
//        因此比较必须用 strncmp(..., 18)，输出必须用 "%.18s"，
//        不能直接用 strcmp 和 %s，否则会越界读到脏数据。
// ============================================================================
#include <iostream>
#include <cstring>  // 必须包含，用于 strcmp / strncmp / strncpy
#include <cstdio>   // 用于 printf / fopen / fscanf / fprintf
#include <cctype>   // 用于 isdigit
#include <iomanip>  // 用于 setw，防止输入过长导致缓冲区溢出
#include <new>      // 用于 std::nothrow（new 分配失败时返回空指针而不是抛异常）

using namespace std;

// ================= 结构体定义 =================
#define SIZE 12      // 定义最大哈希表长度
#define M 11         // 定义小于哈希表长度的最大质数
#define PID_LEN 18   // 身份证号长度，18 位字符

#define BUFF_SIZE 64 // 键盘输入缓冲区大小
#define DATA_FILE "722data.txt"   // 默认的数据文件（可改成绝对路径，如 "D:\\722data.txt"）

typedef char PidType[PID_LEN];   // 定义身份证类型

typedef struct
{
    PidType pid;     // 身份证号码
    int  value;      // 信用值
} KeyType;           // 存放学生信用记录的结构体类型

typedef struct Hnode
{
    KeyType r;              // 学生信用记录的数据域
    struct Hnode *next;     // 指针域，指向下一个记录
} Hnode, *Hlink;            // 链表中的结点类型

Hlink head[SIZE];           // 静态数组 (全局变量)

// ================= 0. 公共小工具函数 =================

// 判断身份证号是否合法：前 17 位是数字，最后 1 位是数字或 'X'
bool Pid_check(const char *id)
{
    if ((int)strlen(id) != PID_LEN)
        return false;

    for (int i = 0; i < PID_LEN; i++)
    {
        if (i == PID_LEN - 1)
        {
            // 第 18 位是校验位，数字或大写 X 都算合法
            if (!isdigit((unsigned char)id[i]) && id[i] != 'X' && id[i] != 'x')
                return false;
        }
        else if (!isdigit((unsigned char)id[i]))
        {
            return false;
        }
    }
    return true;
}

// 比较两个身份证号是否相同
bool Pid_equal(const char *a, const char *b)
{
    return strncmp(a, b, PID_LEN) == 0;
}

// 拷贝身份证号：只拷 18 个字符，不写 '\0'
void Pid_copy(PidType dst, const char *src)
{
    strncpy(dst, src, PID_LEN);
}

// 输出身份证号：精确输出 18 个字符
void Pid_show(const char *id)
{
    printf("%.18s", id);
}

// 由信用值换算信用等级：>=90 为 A，80-89 为 B，70-79 为 C，<70 为 D
char Value_level(int v)
{
    if (v >= 90) return 'A';
    if (v >= 80) return 'B';
    if (v >= 70) return 'C';
    return 'D';
}

// 从键盘读入一个身份证号并检查格式，读入失败或格式错误返回 false
bool Read_pid(char *buf)
{
    if (!(cin >> setw(BUFF_SIZE) >> buf))
    {
        cin.clear();
        cin.ignore(1024, '\n');
        cout << "输入有误！" << endl;
        return false;
    }

    if (!Pid_check(buf))
    {
        cout << "身份证号格式错误（应为 18 位数字）！" << endl;
        return false;
    }
    return true;
}

// ================= 1. 计算哈希地址 =================
int Value_hashtable(PidType id)
{
    // 提取对应的位 (char 减去 '0' 转成数字)
    int combined = (id[3] - '0') * 10000 + (id[9] - '0') * 1000 +
                   (id[11] - '0') * 100 + (id[13] - '0') * 10 + (id[16] - '0');
    return combined % M;
}

// ================= 2. 初始化哈希表 =================
void Init_hashtable()
{
    for (int i = 0; i < SIZE; i++)
    {
        head[i] = nullptr; // C++ 里空指针统一用 nullptr
    }
}

// ================= 3. 哈希查找函数 =================
Hlink Search_hashtable(Hlink head[], PidType id)
{
    int index = Value_hashtable(id);

    Hlink current = head[index];
    while (current != nullptr)
    {
        if (strncmp(current->r.pid, id, PID_LEN) == 0)
        {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

// ================= 4. 插入函数 =================
bool Insert_hashtable(Hlink head[], PidType id, int v)
{
    // 1. 先查重
    if (Search_hashtable(head, id) != nullptr)
    {
        cout << "身份证号码为";
        Pid_show(id);
        cout << "的用户记录已经存在。记录插入失败。" << endl;
        return false;
    }

    // 2. 检查信用值范围
    if (v < 0 || v > 100)
    {
        cout << "信用值应在 0-100 之间，记录插入失败！" << endl;
        return false;
    }

    int index = Value_hashtable(id);

    // 3. 创建新结点
    Hlink newNode = new (nothrow) Hnode;
    if (newNode == nullptr)
    {
        cout << "内存分配失败，记录插入失败！" << endl;
        return false;
    }

    // 4. 数据拷贝
    Pid_copy(newNode->r.pid, id);
    newNode->r.value = v;

    // 5. 头插法入链表
    newNode->next = head[index];
    head[index] = newNode;

    return true;
}

// ================= 5. 数据导入函数 =================
// 文件中每行一条记录，格式为：身份证号 信用值
int data_read(char filename[])
{
    cout << "从指定文件导入数据(" << filename << ")!" << endl;

    FILE *fp = fopen(filename, "r");
    if (fp == nullptr)
    {
        cout << "文件打开失败，请确认文件路径！" << endl;
        return 0;
    }

    char pid[BUFF_SIZE];
    int  v;
    int  count = 0;   // 本次成功导入的记录数
    int  line  = 0;   // 文件行号，便于提示错误位置

    // 依次读入身份证号和信用值，返回值等于 2 表示两个数据都读到了
    while (fscanf(fp, "%63s %d", pid, &v) == 2)
    {
        line++;
        if (!Pid_check(pid))
        {
            cout << "第 " << line << " 行身份证号 " << pid << " 格式错误，该行已跳过！" << endl;
            continue;
        }
        if (Insert_hashtable(head, pid, v))
            count++;
    }

    fclose(fp);
    cout << "文件导入哈希表成功！本次共导入 " << count << " 条记录。" << endl;
    return count;
}

// ================= 6. 数据修改函数 =================
void modify_hashtable(Hlink head[], PidType id)
{
    Hlink p = Search_hashtable(head, id);
    if (p == nullptr)
    {
        cout << "用户记录不存在！" << endl;
        return;
    }

    int v;
    cout << "请更新该同学信用值：";
    if (!(cin >> v))
    {
        cin.clear();
        cin.ignore(1024, '\n');
        cout << "输入有误，修改失败！" << endl;
        return;
    }
    if (v < 0 || v > 100)
    {
        cout << "信用值应在 0-100 之间，修改失败！" << endl;
        return;
    }

    p->r.value = v;
    cout << "用户记录修改成功！" << endl;
}

// ================= 7. 数据删除函数 =================
void del_hashtable(Hlink head[], PidType id)
{
    int index = Value_hashtable(id);

    Hlink p = head[index];
    Hlink prev = nullptr;   // 记录前驱结点，删除时要用

    while (p != nullptr && !Pid_equal(p->r.pid, id))
    {
        prev = p;
        p = p->next;
    }

    if (p == nullptr)
    {
        cout << "用户记录不存在！" << endl;
        return;
    }

    // 摘链：删的是头结点还是中间结点
    if (prev == nullptr)
        head[index] = p->next;
    else
        prev->next = p->next;

    delete p;   // 释放结点（new 出来的必须用 delete 回收）
    cout << "用户记录删除成功！" << endl;
}

// ================= 8. 打印输出函数 =================
// 以链地址的形式输出哈希表中的记录：地址->(身份证号,信用值)->(身份证号,信用值)
void print_hashtable(Hlink head[])
{
    for (int i = 0; i < SIZE; i++)
    {
        printf("%d->", i);
        for (Hlink p = head[i]; p != nullptr; p = p->next)
        {
            printf("(%.18s,%d)", p->r.pid, p->r.value);
            if (p->next != nullptr)
                printf("->");
        }
        printf("\n");
    }
}

// ================= 9. 文件保存函数 =================
void save_hashtable(Hlink head[], char fileName[])
{
    FILE *fp = fopen(fileName, "w");
    if (fp == nullptr)
    {
        cout << "文件保存失败，请确认文件路径！" << endl;
        return;
    }

    int count = 0;
    for (int i = 0; i < SIZE; i++)
    {
        for (Hlink p = head[i]; p != nullptr; p = p->next)
        {
            fprintf(fp, "%.18s %d\n", p->r.pid, p->r.value);
            count++;
        }
    }

    fclose(fp);
    cout << "文件保存成功！本次共保存 " << count << " 条记录。" << endl;
}

// ================= 10. 等级查询功能 =================
void query_hashtable(Hlink head[])
{
    char id[BUFF_SIZE];

    cout << "请输入要查询的学生身份证号：";
    if (!Read_pid(id))
        return;

    Hlink p = Search_hashtable(head, id);
    if (p != nullptr)
    {
        // 查找成功：显示信用等级
        printf("该学生信用等级为%c级！\n", Value_level(p->r.value));
    }
    else
    {
        cout << "查找的用户不存在！" << endl;
    }
}

// ================= 11. 功能菜单函数 =================
void Menu()
{
    cout << "-----------------信用等级查询系统-----------------" << endl;
    cout << "*                                                *" << endl;
    cout << "*      1:数据导入            2:等级查询          *" << endl;
    cout << "*      3:数据修改            4:数据删除          *" << endl;
    cout << "*      5:数据插入            6:打印输出          *" << endl;
    cout << "*      7:文件保存            0:系统退出          *" << endl;
    cout << "*   (>=90:A级, 80-89:B级, 70-79:C级, <70:D级)    *" << endl;
    cout << "--------------------------------------------------" << endl;
}

// ================= 12. 释放哈希表 =================
void Destroy_hashtable()
{
    for (int i = 0; i < SIZE; i++)
    {
        Hlink p = head[i];
        while (p != nullptr)
        {
            Hlink q = p->next;
            delete p;
            p = q;
        }
        head[i] = nullptr;
    }
}

// ================= 主函数 =================
int main()
{
    Init_hashtable();     // 构造哈希表

    int choice;
    char id[BUFF_SIZE];

    // data_read / save_hashtable 的参数是 char[]，而字符串常量是 const char[]，
    // 在 VS 默认的 /permissive- 模式下不能直接把 "722data.txt" 传进去，
    // 所以先把文件名宏拷贝到一个可修改的字符数组里。
    char dataFile[] = DATA_FILE;

    while (true)
    {
        Menu();
        cout << "根据菜单提示进行输入：";
        if (!(cin >> choice))
        {
            cin.clear();
            cin.ignore(1024, '\n');
            cout << "输入有误，请输入 0-7 之间的数字！" << endl;
            continue;
        }

        // 0 号功能：退出系统
        if (choice == 0)
        {
            cout << "系统退出，感谢使用！" << endl;
            break;
        }

        switch (choice)
        {
        case 1:   // 数据导入
            data_read(dataFile);
            break;

        case 2:   // 等级查询
            query_hashtable(head);
            break;

        case 3:   // 数据修改
            cout << "请输入要修改的学生身份证号：";
            if (Read_pid(id))
                modify_hashtable(head, id);
            break;

        case 4:   // 数据删除
            cout << "请输入要删除的学生身份证号：";
            if (Read_pid(id))
                del_hashtable(head, id);
            break;

        case 5:   // 数据插入
        {
            cout << "请输入要增加的学生身份证号和信用值：";
            int v;
            if (cin >> setw(BUFF_SIZE) >> id >> v)
            {
                if (Pid_check(id))
                {
                    if (Insert_hashtable(head, id, v))
                        cout << "用户记录插入成功！" << endl;
                }
                else
                {
                    cout << "身份证号格式错误（应为 18 位数字）！" << endl;
                }
            }
            else
            {
                cin.clear();
                cin.ignore(1024, '\n');
                cout << "输入有误，记录插入失败！" << endl;
            }
            break;
        }

        case 6:   // 打印输出
            print_hashtable(head);
            break;

        case 7:   // 文件保存
            save_hashtable(head, dataFile);
            break;

        default:
            cout << "输入有误，请输入 0-7 之间的数字！" << endl;
            continue;
        }

        cout << "可根据菜单进行下一步操作" << endl;
    }

    Destroy_hashtable();
    return 0;
}
