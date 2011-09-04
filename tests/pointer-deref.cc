#include <iostream>

struct baz {
    int m_a;
    int m_b;

public:
    baz () : m_a (0), m_b(0)
    {
    }

    baz (const baz &a_baz)
    {
        m_a = a_baz.get_a ();
        m_b = a_baz.get_b ();
    }

    int get_a () const {return m_a;}
    void set_a (int a) {m_a = a;}

    int get_b () const {return m_b;}
    void set_b (int b) {m_b = b;}
};//end foo


struct bar {
    baz *m_baz;

public:

    bar () : m_baz (0)
    {
        m_baz = new baz;
    }

    bar (const bar &a_bar)
    {
        if (this == &a_bar) {
            return;
        }
        if (!m_baz) {
            m_baz = new baz;
        }
        if (!a_bar.get_baz ()) {
            return;
        }
        *m_baz = *(a_bar.get_baz ());
    }

    ~bar ()
    {
        if (m_baz) {
            delete m_baz;
            m_baz = 0;
        }
    }

    baz* get_baz () const {return m_baz;}
    void set_baz (baz *a_baz)
    {
        if (m_baz) {
            delete m_baz;
        }
        m_baz = a_baz;
    }
};//end bar

struct foo {
    bar * m_bar;
public:

    foo () : m_bar (0)
    {
        m_bar = new bar;
    }

    foo (const foo &a_foo)
    {
        if (this == &a_foo) {
            return;
        }
        if (!m_bar) {
            m_bar = new bar;
        }
        if (!a_foo.get_bar ()) {
            return;
        }
        *m_bar = *(a_foo.get_bar ());
    }

    ~foo ()
    {
        if (m_bar) {
            delete m_bar;
            m_bar = 0;
        }
    }

    bar* get_bar () const {return m_bar;}
    void set_bar (bar *a_bar)
    {
        if (m_bar) {
            delete m_bar;
        }
        m_bar = a_bar;
    }

};//end struct foo

void
change_baz (baz *a_baz)
{
    if (a_baz) {
        a_baz->set_a (20);
        a_baz->set_b (21);
    }
}

void
change_baz_ptr (baz **a_baz)
{
    baz *my_baz = 0;
    if (a_baz) {
        my_baz = *a_baz;
    }
    if (my_baz) {
        my_baz->set_a (30);
        my_baz->set_b (31);
    }
}

int
main ()
{
    foo *foo_ptr = new foo;
    bar *bar_ptr = new bar;
    baz *baz_ptr = new baz;
    baz b;

    baz_ptr->set_a (10);
    baz_ptr->set_b (11);

    bar_ptr->set_baz (baz_ptr);
    foo_ptr->set_bar (bar_ptr);

    change_baz (baz_ptr);
    change_baz_ptr (&baz_ptr);

    if (foo_ptr->m_bar) {
        ;
    }

    if (foo_ptr->m_bar->m_baz) {
        ;
    }
    
    if (foo_ptr->m_bar->m_baz->m_a) {
        ;
    }


    if (b.m_b) {
        ;
    }

    delete foo_ptr;

    return 0;
}
