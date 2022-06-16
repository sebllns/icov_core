#include "ViewportUnit.h"

NalUnit ViewportUnit::ToNal()
{
    NalUnit nal(icov::enum_t::NalUnitType::NAL_UNIT_CODED_VPU);

    icov::bitstreambuf buf;
    icov::obitstream os(&buf);

    for (int i = 0; i < m_icus.size(); i++)
    {
        icov::int8b icu_size = m_icus[i].GetDataLength();
        os << (icov::byte)icu_size;
        os.write(m_icus[i].GetEncodedData(), icu_size * icu_size);
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
    if (nal.getType() != icov::enum_t::NalUnitType::NAL_UNIT_CODED_VPU)
        return false;

    int size = nal.getRbspLength();
    int position = 0;

    while (position < size)
    {
        int icu_size = (icov::int8b)nal.getRbspByte(position++);
        int icu_pixel_count = (int)(icu_size * icu_size);
        icov::byte *icu_data = new icov::byte[icu_pixel_count];
        nal.readRbsp(icu_data, icu_pixel_count, position);
        position += icu_pixel_count;
        addIcu(InteractiveCodedUnit(icu_data, icu_size));
    }

    return true;
}

void ViewportUnit::addIcu(InteractiveCodedUnit icu)
{
    m_icus.push_back(icu);
}
