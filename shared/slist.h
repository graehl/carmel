// originally from http://pegasus.rutgers.edu/~elflord/cpp/list_howto/ erase_iterator changes to allow insert and erase (not just _after) by Jonathan

#include <iterator>
template <class T>
class slist
{
  struct Node
  {
    Node(const T& x,Node* y = 0):m_data(x),m_next(y){}
    T m_data;
    Node* m_next;
  };

  Node* m_head;
 public:
  typedef T value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  //  typedef unsigned int size_type;
  //  typedef int difference_type;
  //  typedef forward_iterator_tag _Iterator_category;




  class val_iterator 
    : public std::iterator<std::forward_iterator_tag, T>
    {
		
    public:
      //XXX should be private but can't get friend working
      Node* m_rep;
      //		friend class const_iterator;
		
      //		friend class slist;

      inline val_iterator(Node* x=0):m_rep(x){}
      inline val_iterator(const val_iterator& x):m_rep(x.m_rep) {}
      inline val_iterator& operator=(const val_iterator& x)
	{ 
	  m_rep=x.m_rep; return *this; 
	}
      inline val_iterator& operator++()
	{ 
	  m_rep = m_rep->m_next; return *this; 
	}
      inline val_iterator operator++(int)
	{ 
	  val_iterator tmp(*this); m_rep = m_rep->m_next; return tmp; 
	}
      inline typename slist::reference operator*() const { return m_rep->m_data; }
      inline typename slist::pointer operator->() const { return &m_rep->m_data; }
      inline bool operator==(const val_iterator& x) const
	{
	  return m_rep == x.m_rep; 
	}	
      inline bool operator!=(const val_iterator& x) const
	{
	  return m_rep != x.m_rep; 
	}	

    };

  class erase_iterator 
    : public std::iterator<std::forward_iterator_tag, T>
    {
      Node** m_rep; // points to previous node's next pointer (or list's first-node pointer)
    public:
      //friend class const_iterator;
      //		friend class slist;
      //		operator iterator () { return *m_rep; }
      inline erase_iterator(const val_iterator &x) : m_rep(&x.m_rep->m_next) {} // points at following node
      inline erase_iterator(Node** x=0):m_rep(x){}
      inline erase_iterator(const erase_iterator& x):m_rep(x.m_rep) {}
      inline erase_iterator& operator=(const erase_iterator& x)
	{ 
	  m_rep=x.m_rep; return *this; 
	}
      inline erase_iterator& erase_and_advance() {
	Node *killme = *m_rep;
	*m_rep = killme->m_next;
	delete killme;
	return *this;			
      }
      inline erase_iterator& insert(const T& it) {		
	*m_rep = NEW Node(it,*m_rep);
	return *this;
      }
      inline erase_iterator& operator++()
	{ 
	  m_rep = &(*m_rep)->m_next; return *this; 
	}
      inline erase_iterator operator++(int)
	{ 
	  erase_iterator tmp(*this); m_rep = &(*m_rep)->m_next; return tmp; 
	}
      inline typename slist::reference operator*() const { return (*m_rep)->m_data; }
      inline typename slist::pointer operator->() const { return &((*m_rep)->m_data); }
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

  typedef erase_iterator iterator;

  class const_iterator 
    : public std::iterator<std::forward_iterator_tag, const T> 
    {
      const Node* m_rep;
    public:
      friend class iterator;
      friend class slist;

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
	  m_rep = m_rep->m_next; return *this; 
	}
      inline const_iterator operator++(int)
	{ 
	  const_iterator tmp(*this); m_rep = m_rep->m_next; return tmp; 
	}
      inline typename slist::const_reference operator*() const { return m_rep->m_data; }
      inline typename slist::const_pointer operator->() const { return &m_rep->m_data; }
      inline bool operator==(const const_iterator& x) const
	{
	  return m_rep == x.m_rep; 
	}
      inline bool operator!=(const const_iterator& x) const
	{
	  return m_rep != x.m_rep; 
	}



    };


  slist() : m_head(0) {}

  slist(const slist& L) : m_head(0)
    {
      for ( const_iterator i = L.begin(); i!=L.end(); ++i )
	push_front(*i);
      reverse();
    }
  slist(const T &it) : m_head(NEW Node(it,0)) {}
  void reverse()
    {
      Node* p = 0; Node* i = m_head; Node* n;
      while (i)
	{
	  n = i->m_next;
	  i->m_next = p;
	  p = i; i = n;
	}
      m_head = p;
    }

  void swap(slist& x)
    {
      Node* tmp = m_head; m_head = x.m_head; x.m_head = tmp;
    }

  slist& operator=(const slist& x)
    {
      slist tmp(x);
      swap(tmp);
      return *this;
    }

  ~slist() { clear(); }
  void clear() { while (!empty()) pop_front(); }



  inline void push_front(const T&x)
    {
      Node* tmp = NEW Node(x);
      tmp->m_next = m_head;
      m_head = tmp;
    }
  inline void pop_front()
    {
      if (m_head)
	{
	  Node* newhead = m_head->m_next;
	  delete m_head;
	  m_head = newhead;
	}
    }
  inline bool empty() const { return m_head == NULL; }

  inline T& front() { return *begin(); }
  inline const T& front() const { return *begin(); }

  inline val_iterator val_begin() { return val_iterator(m_head); }
  inline val_iterator val_end() { return val_iterator(); }

  //	inline iterator erase_begin() { return begin(); }
  //inline iterator erase_end() { return end(); }
  inline erase_iterator begin() { return erase_iterator(&m_head); }
  inline erase_iterator end() { return erase_iterator(); }
  inline const_iterator begin() const { return const_begin(); }
  inline const_iterator end() const { return const_end(); }
  const_iterator const_begin() const { return const_iterator(m_head); }
  const_iterator const_end() const { return const_iterator(); }


  inline erase_iterator& erase(erase_iterator &e) {
    e.erase_and_advance();
    return e;
  }

  inline erase_iterator& insert(erase_iterator &e,const T& it) {
    return e.insert(it);
    return e;
  }

  void erase_after (iterator& x)
    {
      Node* tmp = x.m_rep->m_next;
      if (x.m_rep->m_next) 
	x.m_rep->m_next = x.m_rep->m_next->m_next;
      delete tmp;
    }

  void insert_after (iterator& x, const T& y)
    {
      Node* tmp = NEW Node(y,x.m_rep->m_next);
      x.m_rep->m_next = tmp;
    }
};


