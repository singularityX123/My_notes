#include "student.h"
#include <vector>
int main()
{ 
    Person p1("张三", 20, 1.7, 80.0);
    Student s1("张三", 20, 1.7, 80.0, 1001, 3);
    p1.print_person_info();
    s1.print_student_info();
}