#include "NDRange.h"

#include <cassert>
#include <cstring>
#include <iostream>

NDRange::NDRange(unsigned work_dim,
		 const size_t *global_work_size,
		 const size_t *global_work_offset,
		 const size_t *local_work_size)
  : work_dim(work_dim) {
  for (unsigned i=0; i<work_dim; i++) {
    m_orig_global_work_size[i] = global_work_size[i];
    m_global_work_size[i] = global_work_size[i];
    m_local_work_size[i] = local_work_size[i];
  }

  if (global_work_offset) {
    for (unsigned i=0; i<work_dim; i++)
      m_offset[i] = global_work_offset[i];
  } else {
    memset(m_offset, 0, 3 * sizeof(size_t));
  }
}

NDRange::NDRange(unsigned work_dim,
		 const size_t *orig_global_work_size,
		 const size_t *global_work_size,
		 const size_t *global_work_offset,
		 const size_t *local_work_size)
  : work_dim(work_dim) {
  for (unsigned i=0; i<work_dim; i++) {
    m_orig_global_work_size[i] = orig_global_work_size[i];
    m_global_work_size[i] = global_work_size[i];
    m_local_work_size[i] = local_work_size[i];
  }

  if (global_work_offset) {
    for (unsigned i=0; i<work_dim; i++)
      m_offset[i] = global_work_offset[i];
  } else {
    memset(m_offset, 0, 3 * sizeof(size_t));
  }
}

NDRange::NDRange(const NDRange &ndRange)
  : work_dim(ndRange.work_dim) {
  for (unsigned i=0; 3; i++) {
    m_orig_global_work_size[i] = ndRange.m_orig_global_work_size[i];
    m_global_work_size[i] = ndRange.m_global_work_size[i];
    m_offset[i] = ndRange.m_offset[i];
    m_local_work_size[i] = ndRange.m_local_work_size[i];
  }
}

NDRange &
NDRange::operator=(const NDRange &ndRange) {
  work_dim = ndRange.work_dim;
  for (unsigned i=0; i<3; i++) {
    m_orig_global_work_size[i] = ndRange.m_orig_global_work_size[i];
    m_global_work_size[i] = ndRange.m_global_work_size[i];
    m_offset[i] = ndRange.m_offset[i];
    m_local_work_size[i] = ndRange.m_local_work_size[i];
  }

  return *this;
}

NDRange::~NDRange() {}

unsigned
NDRange::get_orig_global_size(unsigned dimindx) const {
  assert(dimindx < work_dim);

  return (unsigned) m_orig_global_work_size[dimindx];
}

unsigned
NDRange::get_global_size(unsigned dimindx) const {
  assert(dimindx < work_dim);

  return (unsigned) m_global_work_size[dimindx];
}

unsigned
NDRange::get_local_size(unsigned dimindx) const {
    assert(dimindx < work_dim);

    return (unsigned) m_local_work_size[dimindx];
}

unsigned
NDRange::get_work_dim() const {
  return work_dim;
}

void
NDRange::setOffset(unsigned dimindx, unsigned offset) {
  assert(dimindx < work_dim);

  m_offset[dimindx] = (size_t) offset;
}

unsigned
NDRange::getOffset(unsigned dimindx) const {
  assert(dimindx < work_dim);

  return (size_t) m_offset[dimindx];
}

int
NDRange::splitDim(unsigned dimindx, unsigned nb, unsigned *nominators,
		  unsigned denominator, std::vector<NDRange> *ndRanges) const {
  unsigned nbWorkgroups = (unsigned) m_global_work_size[dimindx] /
    m_local_work_size[dimindx];

  unsigned totalWg = 0;
  for (unsigned i=0; i<nb-1; ++i) {
    size_t global_work_size[work_dim];
    memcpy(global_work_size, m_global_work_size, work_dim * sizeof(size_t));
    unsigned nbWg = (((double) nbWorkgroups * nominators[i]) / denominator);
    global_work_size[dimindx] = nbWg * m_local_work_size[dimindx];
    ndRanges->push_back(NDRange(work_dim, m_orig_global_work_size,
				global_work_size, m_offset, m_local_work_size));
    (*ndRanges)[i].setOffset(dimindx,
			     totalWg * m_local_work_size[dimindx] +
			     m_offset[dimindx]);
    totalWg += nbWg;
  }

  size_t global_work_size[work_dim];
  memcpy(global_work_size, m_global_work_size, work_dim * sizeof(size_t));
  global_work_size[dimindx] = (nbWorkgroups-totalWg) * m_local_work_size[dimindx];
  ndRanges->push_back(NDRange(work_dim, m_orig_global_work_size,
			      global_work_size, m_offset, m_local_work_size));
  (*ndRanges)[nb-1].setOffset(dimindx,
			      totalWg * m_local_work_size[dimindx] +
			      m_offset[dimindx]);

  return 1;
}

int
NDRange::splitDim(unsigned dimindx, int size_gr, double *granu_dscr,
		  std::vector<NDRange> *ndRanges) const {
  unsigned nbWorkgroups = (unsigned) m_global_work_size[dimindx] /
    m_local_work_size[dimindx];

  int nbSplits = size_gr / 3;

  // Compute subndranges
  int totalWg = 0;
  for (int i=0; i<nbSplits-1; ++i) {
    size_t global_work_size[work_dim];
    memcpy(global_work_size, m_global_work_size, work_dim * sizeof(size_t));
    int nbWg = ((double) nbWorkgroups * granu_dscr[i*3+2]);
    global_work_size[dimindx] = nbWg * m_local_work_size[dimindx];
    ndRanges->push_back(NDRange(work_dim, m_orig_global_work_size,
				global_work_size, m_offset, m_local_work_size));
    (*ndRanges)[i].setOffset(dimindx,
			     totalWg * m_local_work_size[dimindx] +
			     m_offset[dimindx]);
    totalWg += nbWg;
  }

  int remainingWgs = nbWorkgroups - totalWg;
  if (remainingWgs < 0) {
    std::cerr << "Error: remaining wgs < 0\n";
    exit(EXIT_FAILURE);
  }

  size_t global_work_size[work_dim];
  memcpy(global_work_size, m_global_work_size, work_dim * sizeof(size_t));
  global_work_size[dimindx] = remainingWgs * m_local_work_size[dimindx];
  ndRanges->push_back(NDRange(work_dim, m_orig_global_work_size,
			      global_work_size, m_offset, m_local_work_size));
  (*ndRanges)[nbSplits-1].setOffset(dimindx,
			      totalWg * m_local_work_size[dimindx] +
			      m_offset[dimindx]);

  return 1;
}

void
NDRange::shiftLeft(unsigned dim, int nbWgs) {
  ssize_t shiftLength = m_local_work_size[dim] * nbWgs;
  ssize_t new_global_size = m_global_work_size[dim] + shiftLength;
  ssize_t new_offset = m_offset[dim] - shiftLength;
  assert(new_global_size > 0);
  assert(new_offset >= 0);
  assert((size_t) new_offset + new_global_size <= m_orig_global_work_size[dim]);
  m_global_work_size[dim] = new_global_size;
  m_offset[dim] = new_offset;
}

void
NDRange::shiftRight(unsigned dim, int nbWgs) {
  ssize_t shiftLength = m_local_work_size[dim] * nbWgs;
  ssize_t new_global_size = m_global_work_size[dim] + shiftLength;
  assert(new_global_size > 0);
  assert(m_offset[dim] + new_global_size <= m_orig_global_work_size[dim]);
  m_global_work_size[dim] = new_global_size;
}

void
NDRange::dump() const {
  std::cerr << "orig global work size : [";
  for (unsigned i=0; i<work_dim-1; ++i)
    std::cerr << m_orig_global_work_size[i] << ", ";
  std::cerr << m_orig_global_work_size[work_dim-1] << "]\n";

  std::cerr << "global work size : [";
  for (unsigned i=0; i<work_dim-1; ++i)
    std::cerr << m_global_work_size[i] << ", ";
  std::cerr << m_global_work_size[work_dim-1] << "]\n";

  std::cerr << "offsets : [";
  for (unsigned i=0; i<work_dim-1; ++i)
    std::cerr << m_offset[i] << ", ";
  std::cerr << m_offset[work_dim-1] << "]\n";

  std::cerr << "local work size : [";
  for (unsigned i=0; i<work_dim-1; ++i)
    std::cerr << m_local_work_size[i] << ", ";
  std::cerr << m_local_work_size[work_dim-1] << "]\n";
}
