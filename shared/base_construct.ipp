
/*

intended to forward constructor args to parent

will be unnecessary in C++11

no include guard (intentional - include it inside your class that definese these:

typedef A self_type;
typedef B base_type;
void init();

generated from ./gen-base_construct.ipp

*/

self_type() : base_type() { init(); }

template <class T1> self_type(T1& t1)
 : base_type(t1) {init();}
template <class T1> self_type(T1 const& t1)
 : base_type(t1) {init();}
template <class T1, class T2> self_type(T1& t1, T2& t2)
 : base_type(t1, t2) {init();}
template <class T1, class T2> self_type(T1& t1, T2 const& t2)
 : base_type(t1, t2) {init();}
template <class T1, class T2> self_type(T1 const& t1, T2& t2)
 : base_type(t1, t2) {init();}
template <class T1, class T2> self_type(T1 const& t1, T2 const& t2)
 : base_type(t1, t2) {init();}
template <class T1, class T2, class T3> self_type(T1& t1, T2& t2, T3& t3)
 : base_type(t1, t2, t3) {init();}
template <class T1, class T2, class T3> self_type(T1& t1, T2& t2, T3 const& t3)
 : base_type(t1, t2, t3) {init();}
template <class T1, class T2, class T3> self_type(T1& t1, T2 const& t2, T3& t3)
 : base_type(t1, t2, t3) {init();}
template <class T1, class T2, class T3> self_type(T1& t1, T2 const& t2, T3 const& t3)
 : base_type(t1, t2, t3) {init();}
template <class T1, class T2, class T3> self_type(T1 const& t1, T2& t2, T3& t3)
 : base_type(t1, t2, t3) {init();}
template <class T1, class T2, class T3> self_type(T1 const& t1, T2& t2, T3 const& t3)
 : base_type(t1, t2, t3) {init();}
template <class T1, class T2, class T3> self_type(T1 const& t1, T2 const& t2, T3& t3)
 : base_type(t1, t2, t3) {init();}
template <class T1, class T2, class T3> self_type(T1 const& t1, T2 const& t2, T3 const& t3)
 : base_type(t1, t2, t3) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1& t1, T2& t2, T3& t3, T4& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1& t1, T2& t2, T3& t3, T4 const& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1& t1, T2& t2, T3 const& t3, T4& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1& t1, T2& t2, T3 const& t3, T4 const& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1& t1, T2 const& t2, T3& t3, T4& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1& t1, T2 const& t2, T3& t3, T4 const& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1& t1, T2 const& t2, T3 const& t3, T4& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1& t1, T2 const& t2, T3 const& t3, T4 const& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1 const& t1, T2& t2, T3& t3, T4& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1 const& t1, T2& t2, T3& t3, T4 const& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1 const& t1, T2& t2, T3 const& t3, T4& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1 const& t1, T2& t2, T3 const& t3, T4 const& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1 const& t1, T2 const& t2, T3& t3, T4& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1 const& t1, T2 const& t2, T3& t3, T4 const& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1 const& t1, T2 const& t2, T3 const& t3, T4& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4> self_type(T1 const& t1, T2 const& t2, T3 const& t3, T4 const& t4)
 : base_type(t1, t2, t3, t4) {init();}
template <class T1, class T2, class T3, class T4, class T5> self_type(T1& t1, T2& t2, T3& t3, T4& t4, T5& t5)
 : base_type(t1, t2, t3, t4, t5) {init();}
template <class T1, class T2, class T3, class T4, class T5, class T6> self_type(T1& t1, T2& t2, T3& t3, T4& t4, T5& t5, T6& t6)
 : base_type(t1, t2, t3, t4, t5, t6) {init();}
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7> self_type(T1& t1, T2& t2, T3& t3, T4& t4, T5& t5, T6& t6, T7& t7)
 : base_type(t1, t2, t3, t4, t5, t6, t7) {init();}
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8> self_type(T1& t1, T2& t2, T3& t3, T4& t4, T5& t5, T6& t6, T7& t7, T8& t8)
 : base_type(t1, t2, t3, t4, t5, t6, t7, t8) {init();}
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9> self_type(T1& t1, T2& t2, T3& t3, T4& t4, T5& t5, T6& t6, T7& t7, T8& t8, T9& t9)
 : base_type(t1, t2, t3, t4, t5, t6, t7, t8, t9) {init();}
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10> self_type(T1& t1, T2& t2, T3& t3, T4& t4, T5& t5, T6& t6, T7& t7, T8& t8, T9& t9, T10& t10)
 : base_type(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10) {init();}
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11> self_type(T1& t1, T2& t2, T3& t3, T4& t4, T5& t5, T6& t6, T7& t7, T8& t8, T9& t9, T10& t10, T11& t11)
 : base_type(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11) {init();}
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12> self_type(T1& t1, T2& t2, T3& t3, T4& t4, T5& t5, T6& t6, T7& t7, T8& t8, T9& t9, T10& t10, T11& t11, T12& t12)
 : base_type(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12) {init();}
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13> self_type(T1& t1, T2& t2, T3& t3, T4& t4, T5& t5, T6& t6, T7& t7, T8& t8, T9& t9, T10& t10, T11& t11, T12& t12, T13& t13)
 : base_type(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13) {init();}
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14> self_type(T1& t1, T2& t2, T3& t3, T4& t4, T5& t5, T6& t6, T7& t7, T8& t8, T9& t9, T10& t10, T11& t11, T12& t12, T13& t13, T14& t14)
 : base_type(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14) {init();}
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15> self_type(T1& t1, T2& t2, T3& t3, T4& t4, T5& t5, T6& t6, T7& t7, T8& t8, T9& t9, T10& t10, T11& t11, T12& t12, T13& t13, T14& t14, T15& t15)
 : base_type(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15) {init();}
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16> self_type(T1& t1, T2& t2, T3& t3, T4& t4, T5& t5, T6& t6, T7& t7, T8& t8, T9& t9, T10& t10, T11& t11, T12& t12, T13& t13, T14& t14, T15& t15, T16& t16)
 : base_type(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16) {init();}
