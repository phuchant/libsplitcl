#ifndef LOCKFREEQUEUE_H
#define LOCKFREEQUEUE_H

class Command;

namespace libsplit {

  // Single Producer & Single Consumer
  class LockFreeQueue {
  public:
    LockFreeQueue(unsigned long size);
    virtual ~LockFreeQueue();

    virtual bool Enqueue(Command* element);
    bool Dequeue(Command** element);
    unsigned long Size();

  protected:
    unsigned long size_;
    volatile Command** elements_;
    volatile unsigned long idx_r_;
    volatile unsigned long idx_w_;
  };

};

#endif /* LOCKFREEQUEUE_H */
