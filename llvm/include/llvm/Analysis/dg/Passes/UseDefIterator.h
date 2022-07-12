#ifndef LLVM_ANALYSIS_DG_PASSES_USEDEFITERATOR_H
#define LLVM_ANALYSIS_DG_PASSES_USEDEFITERATOR_H

template <typename T, typename Obj, typename C>
class ConditionalIterator {
  using cond = bool (Obj::*)(C);
  Obj &O;
  cond Cond;
  T Iter;
  const T End;

public:
  ConditionalIterator(Obj &O, cond Cond, T Begin, T End) : O(O), Cond(Cond), Iter(Begin), End(End) {
    if (Iter != End && !(O.*Cond)(Iter->first))
      getNext();
  }

  ConditionalIterator &operator++() { return getNext(); }
  ConditionalIterator operator++(int) {
    auto Tmp = *this;
    ++(*this);
    return Tmp;
  }

  typename T::reference operator*() { return *Iter; }
  typename T::pointer operator->() { return &(*Iter); }
  friend bool operator==(const ConditionalIterator &Lhs, const ConditionalIterator &Rhs) { return Lhs.Iter == Rhs.Iter; }
  friend bool operator!=(const ConditionalIterator &Lhs, const ConditionalIterator &Rhs) { return Lhs.Iter != Rhs.Iter; }

private:
  ConditionalIterator &getNext() {
    do {
      Iter++;
      if (Iter == End)  break;
    } while(!(O.*Cond)(Iter->first));
    return *this;
  }
};

#endif