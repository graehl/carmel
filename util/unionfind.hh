
template <class Data>
struct UnionNode {
  Data data;
  mutable UnionNode *parent;
  U rank;
  UnionNode(Data const& data=Data()) : data(data), parent(this), rank() {}
  typedef UnionNode *Ptr;
  Ptr repr() const {
    if (parent != this)
      parent = parent->repr();
    return parent;
  }
  Ptr merge(Ptr o) {
    return unionMergeRoots(repr(), o->repr());
  }
  UnionNode & operator += (UnionNode& o) {
    return *merge(&o);
  }
};

template <class PtrT>
PtrT unionMergeRoots(PtrT a, PtrT b) {
  if (a == b) return a;
  if (a->rank < b->rank)
    return a->parent = b;
  else {
    if (a->rank == b->rank)
      ++a->rank;
    return b->parent = a;
  }
}
