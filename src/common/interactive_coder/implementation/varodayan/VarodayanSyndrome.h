#ifndef VARODAYANSYNDROME_H
#define VARODAYANSYNDROME_H

#include "ISyndrome.h"
#include <fstream>
#include <sstream>

namespace icov {

/**************************************************/
struct VarodayanTools : public DefaultSyndromeTools
{
    uint32_t nzmax;
    uint32_t numCodes;
    uint32_t totalNumInc;

    std::vector<uint32_t> numIncVec;
    std::vector<uint32_t> codeSumVec;
    std::vector<uint32_t> txSeq;
    std::vector<uint32_t> jc;
    std::vector<uint32_t> ir;

    Syndrome accumulated_syndrome;


public:
    virtual int get_m(int numcode) const override
    {
        int numInc = this->numIncVec[numcode];
        return (this->get_n() / totalNumInc) * numInc;
    }

    uint32_t getIr(int code, int k) const
    {
        return  ir[code * nzmax + k];
    }

    void read_mat(std::ifstream& file)
    {
        //1- PARSE VARODAYAN FILE
        uint32_t k = 0;
        uint32_t v = 0;

        file >> this->numCodes;
        file >> this->n;
        file >> this->nzmax;
        file >> this->totalNumInc;

        std::cout << numCodes << std::endl;
        std::cout << n << std::endl;
        std::cout << nzmax << std::endl;
        std::cout << totalNumInc << std::endl;

        for (k = 0; k < this->n + 1; k++)
        {
            file >> v;
            this->jc.push_back(v);
        }

        for (uint32_t code = 0; code < this->numCodes; code++)
        {
            file >> v;
            this->numIncVec.push_back(v);

            for (k = 0; k < this->numIncVec[code]; k++)
            {
                file >> v;
                this->txSeq.push_back(v);
            }

            for (k = 0; k < this->nzmax; k++)
            {
                file >> v;
                this->ir.push_back(v);
            }
        }
    }

    void read_mat(const std::string& filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::stringstream message;
            message << "'filename' couldn't be opened ('filename' = " << filename << ").";
            throw message.str();
        }

        read_mat(file);
    }
};

/**************************************************/
class VarodayanAccumulation : public ISyndromeAccumulation<VarodayanTools>
{
public:
    VarodayanAccumulation() = default;

    // ISyndromeAccumulation interface
public:
    virtual void compute(const std::vector<CodedVal> &source_x, VarodayanTools &tool) const override;
};

/**************************************************/
class VarodayanStoredSyndrome : public ISyndromeEncodage<VarodayanTools>
{
public:
    VarodayanStoredSyndrome() = default;


    // ISyndromeEncodage interface
public:
    virtual void compute(Syndrome &o_stored_syndrome, int numcode, const VarodayanTools &tool) const override;
};



/**************************************************/
class VarodayanDeaccumulation : public ISyndromeDecodage<VarodayanTools>
{
public:
    VarodayanDeaccumulation() = default;

    // ISyndromeDeaccumulation interface
public:
    virtual void compute(Syndrome &o_syndrome, const Syndrome &acc_syndrome, int numcode, const VarodayanTools &tool) const override;
};


}
#endif // VARODAYANSYNDROME_H
