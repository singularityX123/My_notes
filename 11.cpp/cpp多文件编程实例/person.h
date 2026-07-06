#ifndef __PERSON_H__
#define __PERSON_H__

#include <iostream>
#include <string>


class Person {
private:
  std::string m_name;
  int m_age;
  double m_height;
  double m_weight;
  double m_bmi;

public:
  Person(std::string name, int age, double height, double weight);
  std::string getName();
  double getBMI();
  void print_person_info();
  ~Person();
};

#endif