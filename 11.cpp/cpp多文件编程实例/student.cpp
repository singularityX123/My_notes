#include "student.h"
using namespace std;

Student::Student(string name,int age, double height, double weight, int student_id, int level)
    : Person(name, age, height, weight)
{
    this->m_student_id = student_id;
    this->m_level = level;
}

void Student::print_student_info()
{
    print_person_info();
    cout << "Student ID: " << m_student_id << endl;
    cout << "Level: " << m_level << endl;
}

Student::~Student()
{
    cout << "Student object destroyed." << endl;
}
