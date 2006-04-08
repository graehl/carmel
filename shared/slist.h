// singly linked list
// originally from http://pegasus.rutgers.edu/~elflord/cpp/list_howto/
// erase_iterator changes to allow insert and erase (not just _after) by Jonathan
#ifndef SLIST_H
# define SLIST_H

#include <iterator>
#include <memory> // for placement new
#include <vector> // for sort
#include <algorithm> // sort
#include <function> // for less
#include <cassert>

#ifdef SLIST_EXTRA_ASSERT
# define slist_assert(x) assert(x)
#else 
# define slist_assert(x)
#endif 

template <class Pred>
struct predicate_indirector
{
    predicate_indirector(Pred const& p) : p(p) {}
    
    template <class It1>
    bool operator()(It1 i1) const
    {
        return p(*i1);
    }
    template <class It1,It2>
    bool operator()(It1 i1, It2 i2) const
    {
        return p(*i1,*i2);
    }
    Pred p;
};

template <class Pred>
predicate_indirector<Pred> indirect_predicate(Pred const& p)
{
    return p;
} 

template <class T>
struct _slist_Node
{
    _slist_Node() {}
    _slist_Node(const T& x,_slist_Node* y = 0):data(x),next(y){}
    T data;
    _slist_Node* next;
};


/// can be copied by value (causes sharing).  does not deallocate (relies on a pool).  use slist if you want ownership/deallocation
/// only works for singleton/static Alloc
template <class T,class Alloc=std::allocator<_slist_Node<T> > >
class slist_shared : private Alloc::template rebind<_slist_Node<T> >::other
{
 public:
  typedef _slist_Node<T> Node;
  Node* head;
    
  typedef T value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef unsigned int size_type;
  //  typedef int difference_type;
  //  typedef forward_iterator_tag _Iterator_category;

  struct val_iterator;
  struct erase_iterator;
  typedef erase_iterator iterator;
    struct const_iterator
        : public std::iterator<std::forward_iterator_tag, const T>
    {
        const Node* m_rep;

        inline const_iterator(const Node* x=0):m_rep(x){}
        inline const_iterator(const const_iterator& x):m_rep(x.m_rep) {}
        inline const_iterator(const val_iterator& x):m_rep(x.m_rep){}
        inline const_iterator& operator=(const const_iterator& x)
        {
            m_rep=x.m_rep; return *this;
        }
        inline const_iterator& operator=(const iterator& x)
        {
            m_rep=x.m_rep; return *this;
        }
        inline const_iterator& operator++()
        {
            m_rep = m_rep->next; return *this;
        }
        inline const_iterator operator++(int)
        {
            const_iterator tmp(*this); m_rep = m_rep->next; return tmp;
        }
        inline typename slist_shared::const_reference operator*() const { return m_rep->data; }
        inline typename slist_shared::const_pointer operator->() const { return &m_rep->data; }
        inline bool operator==(const const_iterator& x) const
        {
            return m_rep == x.m_rep;
        }
        inline bool operator!=(const const_iterator& x) const
        {
            return m_rep != x.m_rep;
        }
    };



    struct val_iterator
        : public std::iterator<std::forward_iterator_tag, T>
    {

        //XXX should be private but can't get friend working
        Node* m_rep;
        //        friend class const_iterator;

        //        friend class slist_shared;

        inline val_iterator(Node* x=0):m_rep(x){}
        inline val_iterator(const val_iterator& x):m_rep(x.m_rep) {}
        inline val_iterator(const const_iterator& x):m_rep(x.m_rep) {}
        inline val_iterator& operator=(const val_iterator& x)
        {
            m_rep=x.m_rep; return *this;
        }
        inline val_iterator& operator++()
        {
            m_rep = m_rep->next; return *this;
        }
        inline val_iterator operator++(int)
        {
            val_iterator tmp(*this); m_rep = m_rep->next; return tmp;
        }
        inline typename slist_shared::reference operator*() const { return m_rep->data; }
        inline typename slist_shared::pointer operator->() const { return &m_rep->data; }
        inline bool operator==(const val_iterator& x) const
        {
            return m_rep == x.m_rep;
        }
        inline bool operator!=(const val_iterator& x) const
        {
            return m_rep != x.m_rep;
        }
    };

    struct node_iterator : public std::iterator<std::forward_iterator_tag, Node *>
    {
        Node* m_rep;
        inline val_iterator(Node* x=0):m_rep(x){}
        inline val_iterator(const val_iterator& x):m_rep(x.m_rep) {}
        inline val_iterator(const const_iterator& x):m_rep(x.m_rep) {}
        inline val_iterator& operator=(const val_iterator& x)
        {
            m_rep=x.m_rep; return *this;
        }
        inline val_iterator& operator++()
        {
            m_rep = m_rep->next; return *this;
        }
        inline val_iterator operator++(int)
        {
            val_iterator tmp(*this); m_rep = m_rep->next; return tmp;
        }
        inline Node * operator*() const { return m_rep; }
        inline Node * operator->() const { return m_rep; } //FIXME: should be an error to take a member of a pointer?
        inline bool operator==(const val_iterator& x) const
        {
            return m_rep == x.m_rep;
        }
        inline bool operator!=(const val_iterator& x) const
        {
            return m_rep != x.m_rep;
        }
        
    };
    inline node_iterator node_begin() { return node_iterator(head); }
    inline node_iterator node_end() { return node_iterator(); }

        
    struct erase_iterator
        : public std::iterator<std::forward_iterator_tag, T>
    {
        Node** m_rep; // points to previous node's next pointer (or list's first-node pointer)
        //friend class const_iterator;
        //        friend class slist_shared;
        //        operator iterator () { return *m_rep; }
        inline erase_iterator(const val_iterator &x) : m_rep(&x.m_rep->next) {} // points at following node
        inline erase_iterator(Node** x=0):m_rep(x){}
        inline erase_iterator(const erase_iterator& x):m_rep(x.m_rep) {}
        inline erase_iterator& operator=(const erase_iterator& x)
        {
            m_rep=x.m_rep; return *this;
        }
        inline erase_iterator& operator++()
        {
            m_rep = &(*m_rep)->next; return *this;
        }
        inline erase_iterator operator++(int)
        {
            erase_iterator tmp(*this); m_rep = &(*m_rep)->next; return tmp;
        }
        inline typename slist_shared::reference operator*() const { return (*m_rep)->data; }
        inline typename slist_shared::pointer operator->() const { return &((*m_rep)->data); }
        // special cases: end-of-list is indicated by == list.end(), but we don't know the address of the last node's next-pointer, so instead we just set list.end().m_rep == 0, which should be considered equal if we're currently pointing (*m_rep) == 0
        //XXX assumes you will never test list.end() == i (everyone writes ++i; i!=end() anyhow)
        inline bool operator==(const erase_iterator& x) const
        {
            // for a == b, either a.p == b.p, or b.p=0 and *a.p = 0
            return (!x.m_rep && !*m_rep) || m_rep == x.m_rep;
        }
        inline bool operator!=(const erase_iterator& x) const
        {
            //return !operator==(x);
            // easier to undestand in terms of ==:
            // for a == b (thus not a != b), either a.p == b.p, or b.p=0 and *a.p = 0
            return (x.m_rep || *m_rep) && m_rep != x.m_rep;
        }
    };

    /// WARNING: O(n) - be very careful to test empty() instead of size()==0
    size_type size() const 
    {
        size_type ret=0;
        for ( const_iterator i = L.begin(); i!=L.end(); ++i )
            ++ret;
        return ret;
    }

    /// inefficient: builds vector of pointers, then rebuilds list
    template <class LT>
    void sort(LT const &lt) 
    {
        std::vector<Node *> v(node_begin(),node_end());
        std::sort(v.begin(),v.end(),indirect_binary_pred(lt));
        reorder(v.begin(),v.end());
    }

    /// must include ALL the nodes on the list (those off will leak)
    template <class Node_ptr_iter>
    static reorder_nodes(Node **pprev_next,Node_ptr_iter i,Node_ptr_iter end)
    {
        Node *previ;
        for(;i!=end;++i)
            pprev_next=&((*pprev_next=*i)->next);
        // have updated next pointer for next round to current i after using previous round's
        (*pprev_next)=NULL; // tie loose end
    }

    /// must include ALL the nodes on the list (those off will leak)
    template <class Node_ptr_iter>
    void reorder(Node_ptr_iter i,Node_ptr_iter end)
    {
        reorder_nodes(&head,i,end);
    }
    
    void sort()
    {
        sort(std::less<T>());
    }
    
    slist_shared() : head(0) {}

    /// shallow copy
    slist_shared(const slist& L) : head(L.head);
    
    slist(Node *n) : head(n) {}
    slist(const T &it) : head(allocate()) {new(head) Node(it,0);}

    void append_deep_copy(Node **pto,Node *from)
    {
        for (from;from=from->next)
            pto=&((*pto=construct(from->data))->next);        
        *pto=NULL;
    }
    
    void construct_deep_copy(const slist_shared &L)
    {
        /*
        head=NULL;
        for ( const_iterator i = L.begin(); i!=L.end(); ++i )
            push_front(*i);
        reverse();
        */
        append_deep_copy(&head,L.head)
    }
    void assign_deep_copy(const slist_shared &L)
    {
        clear();
        construct_deep_copy(L);
    }
    
    inline Node *allocate() 
    {
        return this->allocate(1);
    }
    void deallocate(Node *n)
    {
        this->deallocate(n,1);
    }
    
    void reverse()
    {
        Node* p = 0; Node* i = head; Node* n;
        while (i)
        {
            n = i->next;
            i->next = p;
            p = i; i = n;
        }
        head = p;
    }

    void swap(slist& x)
    {
        Node* tmp = head; head = x.head; x.head = tmp;
    }

    // default operator =: shallow
    
    void clear() { while (!empty()) pop_front(); }

    inline void push_front()
    {
        new(push_front_raw()) T();
    }
    template <class T0>
    inline void push_front(T0 const& t0)
    {
        new(push_front_raw()) T(t0);
    }
    template <class T0,class T1>
    inline void push_front(T0 const& t0,T1 const& t1)
    {
        new(push_front_raw()) T(t0,t1);
    }
    template <class T0,class T1,class T2>
    inline void push_front(T0 const& t0,T1 const& t1,T2 const& t2)
    {
        new(push_front_raw()) T(t0,t1,t2);
    }
    
    T *push_front_raw()
    {
        Node* tmp = allocate();
        tmp->next=head;
        head=tmp;
        return &tmp.data;
    }
    T *push_second_raw()
    {
        slist_assert(!empty());
        Node* tmp = allocate();
        tmp->next=head->next;
        head->next=tmp;
        return &tmp.data;
    }

    
    N *construct() 
    {
        n *ret=allocate();
        new(ret->data)T();
        return ret;
    }
    template <class T0>
    N *construct(T0 t0) 
    {
        n *ret=allocate();
        new(ret->data)T(t0);
        return ret;
    }

    template <class Better_than_pred>
    void push_keeping_front_best(T &t,Better_than_pred better) 
    {
        throw "untested";
        if (empty()) {
            head=construct(t);
            head->next=NULL;
        } else
            add_keeping_front_best(construct(t),better);
    }
    
 private:
    template <class Better_than_pred>
    void add_keeping_front_best(Node *n,Better_than_pred better)
    {
        throw "untested";
        
        slist_assert(!empty());
        if (better(head->data,n->data)) {
            n->next=head->next;
            head->next=n;
        } else {
            n->next=head;
            head=n;
        }
    }
    public:
            
    inline void pop_front()
    {
        if (head)
        {
            Node* newhead = head->next;
            deallocate(head);
            head = newhead;
        }
    }
    Node *head_node() const 
    {
        return head;
    }
    Node *&head_node()
    {
        return head;
    }
    inline bool empty() const { return head == NULL; }

    inline T& front() { return *begin(); }
    inline const T& front() const { return *begin(); }

    inline val_iterator val_begin() { return val_iterator(head); }
    inline val_iterator val_end() { return val_iterator(); }

    // defined in list.h instead
    //inline iterator erase_begin() { return begin(); }
    //inline iterator erase_end() { return end(); }
    inline erase_iterator begin() { return erase_iterator(&head); }
    inline erase_iterator end() { return erase_iterator(); }
    inline const_iterator begin() const { return const_begin(); }
    inline const_iterator end() const { return const_end(); }
    const_iterator const_begin() const { return const_iterator(head); }
    const_iterator const_end() const { return const_iterator(); }


    inline erase_iterator& erase(erase_iterator &e) {
        Node *killme = *e.m_rep;
        *e.m_rep = killme->next;
        deallocate(killme);
        return e;
    }

    inline erase_iterator& insert(erase_iterator &e,const T& it) { // moves iterator back to inserted thing!
        Node *prev=allocate();
        new (prev)Node(it,*e.m_rep);
        *e.m_rep=prev;
        return e;
    }

    void erase_after (iterator& x)
    {
        Node* tmp = x.m_rep->next;
        if (x.m_rep->next)
            x.m_rep->next = x.m_rep->next->next;
        deallocate(tmp);
    }

    iterator insert_after (iterator& x, const T& y)
    {
        Node* tmp = allocate();
        new(tmp) Node(y,x.m_rep->next);
        x.m_rep->next = tmp;
        return iterator(tmp);
    }
};


template <class T,class Alloc=std::allocator<_slist_Node<T> > >
class slist : public slist_shared<T,Alloc>
{
 public:
    typedef slist_shared<T,Alloc> slist_base;
    
    slist() : slist_base(slist) {}
    slist(slist_base const& L) { this->construct_deep_copy(L); }
    slist(Node *n) : slist_base(n) {}
    slist(const T &it) : slist_base(it) {}
    slist_shared& operator=(slist_base const& x)
    {
        this->assign_deep_copy(x);
        return *this;
    }
    ~slist() { this->clear(); }
};

    
#endif
