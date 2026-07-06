#ifndef __STUDENT_H__
#define __STUDENT_H__

#include "person.h"

class Student: public Person
{
private:
    int m_student_id;
    int m_level;

public:
    Student(std::string name,int age, double height, double weight, int student_id, int level);
    void print_student_info();
    ~Student();
};

#endif