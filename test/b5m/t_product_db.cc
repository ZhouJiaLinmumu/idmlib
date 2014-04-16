#include <idmlib/duplicate-detection/psm.h>
#include <util/ustring/UString.h>
#include <boost/lexical_cast.hpp>
#include <util/ustring/UString.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <idmlib/b5m/comment_db.h>
#include <idmlib/b5m/product_db.h>
#include <util/ustring/UString.h>
#include <iostream>
#include <map>
#include <stdio.h>
#include <boost/test/unit_test.hpp>
#include <stdlib.h>
using namespace idmlib::b5m;
using namespace std;
using namespace boost;
ProductProperty pp;
ProductProperty another;
ProductProperty third;


BOOST_AUTO_TEST_SUITE(Productdb_test)
/*
SourceA
产地:中国,质量:优
AddbeforToStringpid:46c999f5d10830d0c59487bd48adce8a,DATE:20130301,itemcount:1,price:1000.00-3213.00,source:[SourceA]
ToStringpid:46c999f5d10830d0c59487bd48adce8a,DATE:20130301,itemcount:2,price:100.00-3213.00,source:[SourceB][SourceA]
GetAttributeUString产地:美国,品牌:阿迪王,质量:优
ToString
pid:46c999f5d10830d0c59487bd48adce8a,DATE:20130529,itemcount:2,price:100.00-1000213.00,source:[SourceB][SourceC]
GetAttributeUString产地:美国,品牌:阿迪王,质量:优
ToStringpid:46c999f5d10830d0c59487bd48adce8a,DATE:20130529,itemcount:4,price:100.00-1000213.00,source:[SourceB][SourceA][SourceC]
GetAttributeUString产地:美国,品牌:阿迪王,质量:优
*/
BOOST_AUTO_TEST_CASE(Product)
{

    Document doc;
    string spid="7bc999f5d10830d0c59487bd48a73cae",soid="46c999f5d10830d0c59487bd48adce8a",source="SourceA",date="20130301",price="1000~3213",attribute="产地:中国,质量:优";//spid和soid反了
    doc.property("DATE") = str_to_propstr(date, izenelib::util::UString::UTF_8);
    doc.property("DOCID") = str_to_propstr(spid);
    doc.property("uuid") = str_to_propstr(soid);
    doc.property("Price") = str_to_propstr(price,izenelib::util::UString::UTF_8);;
    doc.property("Source") = str_to_propstr(source,izenelib::util::UString::UTF_8);
    doc.property("Attribute") = str_to_propstr(attribute,izenelib::util::UString::UTF_8);;

    pp.Parse(doc);

    spid="12345f5d10830d0c59487bd48a73cae",soid="46c999f5d10830d0c59487bd48adce8a",source="SourceB",date="20130229",price="100~1213",attribute="产地:美国,质量:优,品牌:阿迪王";
    doc.property("DATE") =  str_to_propstr(date, izenelib::util::UString::UTF_8);
    doc.property("DOCID") = str_to_propstr(spid,izenelib::util::UString::UTF_8);
    doc.property("uuid") =  str_to_propstr(soid,izenelib::util::UString::UTF_8);
    doc.property("Price") = str_to_propstr(price,izenelib::util::UString::UTF_8);
    doc.property("Source") = str_to_propstr(source,izenelib::util::UString::UTF_8);
    doc.property("Attribute") = str_to_propstr(attribute,izenelib::util::UString::UTF_8);
    another.Parse(doc);
    BOOST_CHECK_EQUAL(pp.GetSourceString(),"SourceA");

    izenelib::util::UString AttributeUString=pp.GetAttributeUString();
   
    string Attribute;
    AttributeUString.convertString(Attribute, izenelib::util::UString::UTF_8);

    BOOST_CHECK_EQUAL(Attribute,"产地:中国,质量:优");
    BOOST_CHECK_EQUAL(pp.ToString(),"pid:46c999f5d10830d0c59487bd48adce8a,DATE:20130301,itemcount:1,price:1000.00-3213.00,source:[SourceA]");
    pp+=another;


    BOOST_CHECK_EQUAL(pp.ToString(),"pid:46c999f5d10830d0c59487bd48adce8a,DATE:20130301,itemcount:2,price:100.00-3213.00,source:[SourceB][SourceA]");
    izenelib::util::UString ua=pp.GetAttributeUString();
    ua.convertString(attribute, izenelib::util::UString::UTF_8);
    BOOST_CHECK_EQUAL(attribute,"产地:美国,品牌:阿迪王,质量:优");

    spid="12345f5d54433d0c59487bd48a73cae",soid="46c999f5d10830d0c59487bd48adce8a",source="SourceC",date="20130529",price="5000~1000213",attribute="产地:美国,质量:差,品牌:阿迪王";
    doc.property("DATE") =   str_to_propstr(date, izenelib::util::UString::UTF_8);
    doc.property("DOCID") =  str_to_propstr(spid,izenelib::util::UString::UTF_8);
    doc.property("uuid") =   str_to_propstr(soid,izenelib::util::UString::UTF_8);
    doc.property("Price") =  str_to_propstr(price,izenelib::util::UString::UTF_8);
    doc.property("Source") = str_to_propstr(source,izenelib::util::UString::UTF_8);
    doc.property("Attribute") = str_to_propstr(attribute,izenelib::util::UString::UTF_8);
    third.Parse(doc);
    third+=another;



    BOOST_CHECK_EQUAL(third.ToString(),"pid:46c999f5d10830d0c59487bd48adce8a,DATE:20130529,itemcount:2,price:100.00-1000213.00,source:[SourceB][SourceC]");
    izenelib::util::UString ua1=third.GetAttributeUString();
    ua1.convertString(attribute, izenelib::util::UString::UTF_8);
    BOOST_CHECK_EQUAL(attribute,"产地:美国,品牌:阿迪王,质量:优");

    pp+=third;

    //BOOST_CHECK_EQUAL(pp.ToString(),"pid:46c999f5d10830d0c59487bd48adce8a,DATE:20130529,itemcount:4,price:100.00-1000213.00,source:[SourceB][SourceA][SourceC]");
    izenelib::util::UString ua2=pp.GetAttributeUString();
    ua2.convertString(attribute, izenelib::util::UString::UTF_8);
    BOOST_CHECK_EQUAL(attribute,"产地:美国,品牌:阿迪王,质量:优");
    //pp.Set(value.doc);

}

BOOST_AUTO_TEST_SUITE_END()
