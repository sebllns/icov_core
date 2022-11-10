#ifndef INTERACTIVECODER_H
#define INTERACTIVECODER_H

#include <memory>

#include "BitSymbolProvider.h"
#include "LLRProvider.h"
#include "common.h"
#include "interface/IDecoder.h"
#include "interface/IMatrixProvider.h"
#include "interface/ISyndrome.h"

namespace icov {

template <class Matrix> class InteractiveCoder {
  using IMatrixProvider_sp = std::shared_ptr<IMatrixProvider<Matrix>>;
  using IDecoder_sp = std::shared_ptr<IDecoder<Matrix>>;
  using ISyndromeManager_sp = std::shared_ptr<ISyndromeManager>;
  using LLRProvider_sp = std::shared_ptr<LLRProvider>;

public:
  InteractiveCoder(IDecoder<Matrix> *dec, IMatrixProvider<Matrix> *mp,
                   ISyndromeManager *syndromeMgr, LLRProvider *llrProvider)
      : m_pdecoder(dec), m_psyndromeMgr(syndromeMgr),
        m_pllrProvider(llrProvider), m_matrix_provider(mp) {
    m_decodage_syndrome =
        std::unique_ptr<Syndrome>(new Syndrome(syndromeMgr->get_n()));
  };

  InteractiveCoder(const IDecoder_sp &dec, const IMatrixProvider_sp &mp,
                   const ISyndromeManager_sp &syndromeMgr,
                   const LLRProvider_sp &llrProvider)
      : m_pdecoder(dec), m_psyndromeMgr(syndromeMgr),
        m_pllrProvider(llrProvider), m_matrix_provider(mp) {
    m_decodage_syndrome =
        std::unique_ptr<Syndrome>(new Syndrome(syndromeMgr->get_n()));
  };

  ~InteractiveCoder() = default;

  size_t get_n() const;

  Rate getRate(const NumcodeIdx &numcodeIdx) const;

  bool testConvergence(const BitVec &decoded, const BitVec &source) const;

  int errorCount(const BitVec &decoded, const BitVec &source) const;

  int encode(NumcodeIdx &io_numcode, Syndrome &o_stored_syndrome,
             const BitVec &source);

  int encode(uint32_t &io_numcode, Syndrome &o_stored_syndrome,
             const BitVec &source, uint32_t numcodeMax, uint32_t step);

  void encode(std::vector<SINumcodeIdx> &rates,
              std::vector<SISyndromes> &syndromes, const std::vector<SIVec> &si,
              const BitSymbolProvider::params &p, const SymboleVec &src);

  void encode_bitplanes(const BitSymbolProvider &bsp, SINumcodeIdx &rate,
                        SISyndromes &stored_synd);

  void encode_bitplane(const BitSymbolProvider &bsp, int bitplane_idx,
                       NumcodeIdx &numcodeIdx, Syndrome &stored_synd,
                       SymboleVec &predec);

  void decode(BitVec &o_decoded, NumcodeIdx numcodeIdx,
              const Syndrome &syndrome);
  void decode_bitplanes(SymboleVec &output, const SINumcodeIdx &numcodes,
                        const SISyndromes &stored_synd);

  void init_LLR_bitplane(BitSymbolProvider bsp, int bitplane_idx);

  IMatrixProvider_sp matrixProvider() const { return this->m_matrix_provider; }
  IDecoder_sp decoder() const { return this->m_pdecoder; }
  ISyndromeManager_sp syndromeMgr() const { return this->m_psyndromeMgr; }
  LLRProvider_sp llrProvider() const { return this->m_pllrProvider; }

protected:
  IMatrixProvider_sp m_matrix_provider;
  IDecoder_sp m_pdecoder;
  ISyndromeManager_sp m_psyndromeMgr;
  LLRProvider_sp m_pllrProvider;

  std::unique_ptr<Syndrome> m_decodage_syndrome;
  BitVec m_decoded_bits;
};

} // namespace icov

#include "InteractiveCoder.hpp"

#endif // INTERACTIVECODER_H
