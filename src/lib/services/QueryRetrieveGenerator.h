/*************************************************************************
 * dopamine - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#ifndef _76b2ab31_bb60_4366_ad0b_55c6eb286fdf
#define _76b2ab31_bb60_4366_ad0b_55c6eb286fdf

#include <dcmtk/config/osconfig.h> /* make sure OS specific configuration is included first */
#include <dcmtk/dcmdata/dctagkey.h>

#include "Generator.h"
#include "ServicesTools.h"

namespace dopamine
{

namespace services
{

class QueryRetrieveGenerator : public Generator
{
public:
    QueryRetrieveGenerator(std::string const & username,
                           std::string const & service_name);

    virtual Uint16 set_query(mongo::BSONObj const & query_dataset);

    DcmTagKey get_instance_count_tag() const;

    bool get_convert_modalities_in_study() const;

    std::string get_query_retrieve_level() const;

    void set_includefields(std::vector<std::string> includefields);

protected:
    std::string _service_name;

    std::string _query_retrieve_level;

    DcmTagKey _instance_count_tag;

    /// flag indicating if modalities should be convert
    bool _convert_modalities_in_study;

private:
    std::vector<std::string> _includefields;

};

} // namespace services

} // namespace dopamine

#endif // _76b2ab31_bb60_4366_ad0b_55c6eb286fdf