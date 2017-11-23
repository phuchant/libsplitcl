#ifndef NDRANGE_H
#define NDRANGE_H

#include <cstdlib>
#include <vector>

class NDRange {
public:
  NDRange(unsigned work_dim,
	  const size_t *global_work_size,
	  const size_t *global_work_offset,
	  const size_t *local_work_size);
  NDRange(unsigned work_dim,
	  const size_t *orig_global_work_size,
	  const size_t *global_work_size,
	  const size_t *global_work_offset,
	  const size_t *local_work_size);
  NDRange(const NDRange &ndRange);

  NDRange & operator=(const NDRange &ndRange);

  ~NDRange();

  unsigned get_orig_global_size(unsigned dimindx) const;
  unsigned get_global_size(unsigned dimindx) const;
  unsigned get_local_size(unsigned dimindx) const;
  unsigned get_work_dim() const;

  void setOffset(unsigned dimindx, unsigned offset);
  unsigned getOffset(unsigned dimindx) const;
  int splitDim(unsigned dimindx, unsigned nb, unsigned *nominators,
	       unsigned denominator, std::vector<NDRange> *ndRanges) const;
  int splitDim(unsigned dimindx, int size_dscr, double *granu_dscr,
	       std::vector<NDRange> *ndRanges) const;
  void shiftLeft(unsigned dim, int nbWgs);
  void shiftRight(unsigned dim, int nbWgs);

  void dump() const;

private:
  unsigned work_dim;
  size_t m_orig_global_work_size[3];
  size_t m_global_work_size[3];
  size_t m_offset[3];
  size_t m_local_work_size[3];
};

#endif /* NDRANGE_H */
