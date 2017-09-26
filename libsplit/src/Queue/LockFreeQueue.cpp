#include <Queue/Command.h>
#include <Queue/LockFreeQueue.h>

namespace libsplit {

  LockFreeQueue::LockFreeQueue(unsigned long size) {
    size_ = size;
    idx_r_ = 0;
    idx_w_ = 0;
    elements_ = (volatile Command**)(new Command*[size]);
  }

  LockFreeQueue::~LockFreeQueue() {
    delete[] elements_;
  }

  bool LockFreeQueue::Enqueue(Command* element) {
    unsigned long next_idx_w = (idx_w_ + 1) % size_;
    if (next_idx_w == idx_r_) return false;
    elements_[idx_w_] = element;
    __sync_synchronize();
    idx_w_ = next_idx_w;
    return true;
  }

  bool LockFreeQueue::Dequeue(Command** element) {
    if (idx_r_ == idx_w_) return false;
    unsigned long next_idx_r = (idx_r_ + 1) % size_;
    *element = (Command*) elements_[idx_r_];
    idx_r_ = next_idx_r;
    return true;
  }

  unsigned long LockFreeQueue::Size() const {
    if (idx_w_ >= idx_r_) return idx_w_ - idx_r_;
    return size_ - idx_r_ + idx_w_;
  }

};
