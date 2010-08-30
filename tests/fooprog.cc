#include <string>
#include <iostream>

void
func1_1 (int i_i)
{
  ++i_i;
}

void
func1 ()
{
    int i = 17;
    ++i;
    func1_1 (i);
}

void
func2 (int a_a, int a_b)
{
    int j = a_a;
    ++j;
    j = j + a_b;
}


struct Person {
    std::string m_first_name;
    std::string m_family_name;
    unsigned m_age;

    Person () :
        m_age (0)
    {
    }

    Person (const std::string &a_first_name,
               const std::string &a_family_name,
               unsigned a_age)
    {
        m_first_name = a_first_name;
        m_family_name = a_family_name;
        m_age = a_age;
    }

    const std::string& get_first_name () const {return m_first_name;}
    void set_first_name (const std::string &a_name) {m_first_name = a_name;}

    const std::string& get_family_name () const {return m_family_name;}
    void set_family_name (const std::string &a_name) {m_family_name = a_name;}

    int get_age () const {return m_age;}
    void set_age (int a_age) {m_age = a_age;}

    void do_this ()
    {
        std::string foo = "something";
        foo += " good";
        std::string bar = " can";

        foo += bar;
    }

    void overload ()
    {
        int i = 0;
        ++i;
    }

    void overload (int i)
    {
        ++i;
        i= 0;
    }
};//class Person

void
func3 (Person &a_param)
{
    a_param.do_this ();
}

static int i,j;
void
func4 (Person &a_person)
{
    i=0;
    for (j=0; j < 1000; ++j) {
        i = j;
        a_person.overload (i);
    }
}

int
main (int a_argc __attribute__((unused)),
      char *a_argv[] __attribute__((unused)))
{
    Person person ("Bob", "Barton", 15);
    func1 ();

    func2 (1, 2);

    person.set_first_name ("Ali");
    person.set_family_name ("BABA");
    person.do_this ();
    person.overload ();
    person.overload (0);

    func3 (person);

    func4 (person);

    return 0;
}

