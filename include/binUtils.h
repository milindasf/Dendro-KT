
/**
  @file binUtils.h
  @brief A set of efficient functions that use binary operations to perform some small computations.
  @author Hari Sundar, hsundar@gmail.com 
  */

#ifndef __BIN_UTILS_H_
#define __BIN_UTILS_H_
#include <vector>
#include <array>

/**
  @namespace binOp
  @brief A set of functions for fast binary operations.
  @author Hari Sundar
  */
namespace binOp{

  /**
    @return true if n is a power of 2.
    */
  bool isPowerOfTwo(unsigned int n);

  /**
    @return the minimum number of digits required to represent num in binary
    */
  unsigned int binLength(unsigned int num) ;

  /**
    return log to base 2 of num
    */
  unsigned int fastLog2(unsigned int num) ;

  /**
    @brief Converts a decimal number to binary
    @param dec the decimal number
    @param binLen the number of digits required in the binary representation
    @param result the binary representation
    @return error flag
    */
  int toBin(unsigned int dec, unsigned int binLen,  std::vector<bool>& result);

  /**
    @param numBin binary representation of the number
    @param binLen length of numBin
    @return the decimal representation of the binary number
    */
  unsigned int binToDec(unsigned int* numBin, unsigned int binLen) ;

  /**
    @return compute the next highest power of 2 of 32-bit v
    */
  int getNextHighestPowerOfTwo(unsigned int n);

  /**
    @return  compute the prev highest power of 2 of 32-bit v
    */
  int getPrevHighestPowerOfTwo(unsigned int n);

  /**
    @brief Finds the 0-based index of the least-significant 1, starting from least significant bits.
    @author Masado Ishii
    */
  unsigned int lowestOnePos(unsigned int num);

  /**@brief sets the i^th bit on the value val*/
  template <typename T>
  inline void setBit(T& val, unsigned int i)
  {
    val=(((T)1<<i)|val);
  }

  /**@brief gets the i^th bit on the value val*/
  template<typename T>
  inline unsigned int getBit(T val, unsigned int i)
  {
    return (val >> i) & 1;
  }


  /**
   * @brief Expand/collapse bits in a bit string.
   * @description TallBitMatrix expands bits in a bit string.
   * @tparam W width of the tall matrix.
   * @tparam B underlying type representing bit strings.
   */
  template <unsigned char W, typename B = unsigned char>
  class TallBitMatrix
  {
    protected:
      std::array<B, W> m_columns;
      unsigned char m_numNonzeroColumns;

    public:
      void clear() { m_columns.fill(0);  m_numNonzeroColumns = 0; }

      /**@brief Use the places of the first W set bits as basis vectors in a matrix. */
      TallBitMatrix static generateColumns(B ones)
      {
        TallBitMatrix M;
        M.clear();
        B new_col = 1u;
        int c = 0;
        while (new_col && c < W)
        {
          if (ones & new_col)
            M.m_columns[c++] = new_col;
          new_col <<= 1;
        }
        M.m_numNonzeroColumns = c;
        return M;
      }

      /**@brief Performs matrix multiplication, i.e. inserts zeroed places into the string. */
      B expandBitstring(B vec)
      {
        B vecInSubspace = 0u;
        vec &= ((1u << m_numNonzeroColumns) - 1u);
        int c = 0;
        while (vec)
        {
          vecInSubspace ^= m_columns[c++] & (0u - bool(vec & 1u));
          vec >>= 1;
        }
        return vecInSubspace;
      }
  };

}//end namespace

#endif
