#include "../slist.h"
#include <iostream>
#include <algorithm>
using namespace std;

typedef slist<int> List;
typedef List::Node Node;

ostream & operator <<(ostream &out,const List &list) 
{
    ostream_iterator<int> ostart (out,"\n");
    copy(list.begin(),list.end(),ostart);
}

Node * reverse_nodes_r(Node *p, Node *prev=NULL) 
{
    if(!p) return p;
    Node *n=p->next;
    p->next=prev;
    if (!n) return p;
    return reverse_nodes_r(n,p);
}

Node * reverse_nodes(Node *p) 
{
    Node *next;
    Node *prev = NULL;
    while(p) {
        next=p->next;
        p->next=prev;
        prev=p;
        p=next;
    }
    return prev;
}

// returns pointer to first node with data=key
Node *find_first(Node *p,int key)
{
    while(p) {
        if(p->data == key)
            return p;
        p=p->next;
    }
    return NULL;
}

Node *find_last(Node *p,int key)
{
    Node *last=NULL;
    while(p) {
        if(p->data == key)
            last=p;
        p=p->next;
    }
    return last;
}

Node *find_last_r(Node *p, int key)
{
    if (!p) return p;
    Node *f=find_last_r(p->next,key);
    if (f) return f;
    return (p->data == key) ? p : NULL;
}

Node *copy_reversed(Node *p, Node *&copied_end /*OUTPUT*/)
{    
    if(!p) return p;
    copied_end=new Node(p->data,NULL);
    if (p->next) {        
        Node *e;
        Node *n=copy_reversed(p->next,e);
        e->next=copied_end;
        return n;
    } else {
        return copied_end;
    }    
}

void reverse_inplace(List &list) 
{
    list.head=reverse_nodes(list.head);
}

void print_list(ostream &out,Node *p) 
{
    while(p) {    
        out << p->data << ' ';
        p=p->next;
    }    
}

ostream & operator <<(ostream &out,Node *p) 
{
    print_list(out,p);
    return out;
}

int main() 
{
    int key;
    cin >> key;    
    
    List l;
    typedef front_insert_iterator<List> Fronter;
    
    Fronter insert(l);
    cerr << "Input a list of integers (then EOF)\n";
    istream_iterator<int> istart (cin),iend;
    
    copy(istart,iend,insert);
    cerr << "Your reversed input was: " << l.head << endl;

    List r;
    Fronter rfront(r);
    copy(l.begin(),l.end(),rfront);
    cerr << "(reversed back):"<<r.head << endl;

    reverse_inplace(l);
    cerr << "(reversed in place):"<<l.head << endl;

    Node *copied,*copied_end;    
    copied=copy_reversed(l.head,copied_end);
    cerr << "(reversed copy):" << copied << endl;

    Node *found=find_first(copied,key);
    cerr << "Looking for "<<key<<" in list: "<<copied<<endl;
    
    if (found) {        
        cerr << key << " found in list: " << found << endl;
        found=find_last(copied,key);
        cerr << key << " found (LAST) in list: " << found << " (recursive:) " << find_last_r(copied,key) << endl;
    } else
        cerr << key << " not found in list (" << copied << ")"<< endl;
}
