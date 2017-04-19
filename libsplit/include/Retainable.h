#ifndef RETAINABLE_H
#define RETAINABLE_H

class Retainable {
public:
  Retainable() : ref_count(1) {}
  virtual ~Retainable() {}

  void retain() {
    ref_count++;
  }

  virtual bool release() {
    return --ref_count == 0;
  }

protected:
  unsigned ref_count;
};

#endif /* RETAINABLE_H */
