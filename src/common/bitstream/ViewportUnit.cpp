#include "ViewportUnit.h"

ViewportUnit::ViewportUnit()
{
}

NalUnit ViewportUnit::ToNal()
{
    NalUnit nal(icov::NalUnitType::NAL_UNIT_CODED_VPU);

    icov::bitstreambuf buf;
    icov::obitstream os(&buf);

    for (int i = 0; i < m_icus.size(); i++)
    {
        os << (icov::byte)m_icus[i].GetDataLength();
        os.write(m_icus[i].GetEncodedData(), m_icus[i].GetDataLength());
    }

    //std::cout << os.rdbuf()->in_avail() << std::endl;
    //std::cout << os.tellp() << std::endl;
    //std::cout << buf.size() << std::endl;

    icov::byte *data = new icov::byte[buf.size()];
    buf.sgetn(data, buf.size());

    nal.setRbsp(data, buf.size());

    return nal;
}

bool ViewportUnit::TryParse(NalUnit nal)
{
    if (nal.getType() != icov::NalUnitType::NAL_UNIT_CODED_VPU)
        return false;

    int size = nal.getRbspLength();
    int position = 0;

    while(position < size)
    {
        int icu_size = (int)nal.getRbspByte(position++);   
        icov::byte* icu_data = new icov::byte[icu_size];
        nal.readRbsp(icu_data, icu_size, position);
        position += icu_size;
        addIcu(InteractiveCodedUnit(icu_data, (icov::int8b)icu_size));
    }
  

    return true;
}

void ViewportUnit::addIcu(InteractiveCodedUnit icu)
{
    m_icus.push_back(icu);
}
