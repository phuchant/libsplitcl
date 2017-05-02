#ifndef RETAINABLE_H
#define RETAINABLE_H

namespace libsplit {

  class Retainable {
  public:
    Retainable() {
      refCount = 1;
    }

    virtual ~Retainable() {}

    void retain() {
      int curRefCount;
      do {
	curRefCount = refCount;
      } while (!__sync_bool_compare_and_swap(&refCount, curRefCount,
					     curRefCount + 1));
    }

    void release() {
      int curRefCount;
      do {
	curRefCount = refCount;
      } while (!__sync_bool_compare_and_swap(&refCount, curRefCount,
					     curRefCount - 1));
      if (curRefCount == 1)
	delete this;
    }

  private:
    int refCount;
  };

};

#endif /* RETAINABLE_H */
