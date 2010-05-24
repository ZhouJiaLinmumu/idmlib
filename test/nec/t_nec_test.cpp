/*
 * t_ner_test.cpp
 *
 *  Created on: Mar 30, 2010
 *      Author: eric
 */
#include "t_nec.h"
#include <idmlib/nec/NameEntity.h>
#include <idmlib/nec/NameEntityUtil.h>
#include <ml/Evaluator.h>
#include <idmlib/nec/NameEntityDict.h>
#include <idmlib/nec/NameEntityManager.h>
#include <fstream.h>
#include <string>
#include <set>
#include <idmlib/util/StringUtil.hpp>

using namespace idmlib;

void loadDict(const std::string& dictName, std::map<std::string, int>& dict)
{
	std::cout<<"load Dict"<<std::endl;
	ifstream inStream(dictName.c_str());
	std::string line = "";
	while (std::getline(inStream, line))
	{
		if (line.length() > 0)
		{
			int posEnd=line.find(" ");
			string cur = line.substr(0, posEnd);
			string strlabel=line.substr(posEnd+1, line.size()-posEnd-1);
//			std::cout<<cur<<","<<strlabel<<std::endl;
			int count=atoi(strlabel.c_str());
			dict.insert(make_pair(cur, count));
		}
	}

}

void loadDict(const std::string& dictName, std::map<std::string, int>& dict, int thresh)
{
	std::cout<<"load Dict"<<std::endl;
	ifstream inStream(dictName.c_str());
	std::string line = "";
	while (std::getline(inStream, line))
	{
		if (line.length() > 0)
		{
			int posEnd=line.find(" ");
			string cur = line.substr(0, posEnd);
			string strlabel=line.substr(posEnd+1, line.size()-posEnd-1);
			int count=atoi(strlabel.c_str());
			if(count>=thresh)
				dict.insert(make_pair(cur, count));
		}
	}

}

void testNec()
{
    std::ofstream testOut("necResult.txt");
	std::string test_path = "../db/nec/test/test.txt";
//    std::string test_path = "../db/nec/test/engtest";
	std::string path = "../resource/nec/";
	NameEntityManager neMgr(path);

	cout << "loading entities..." << endl;
	std::vector<NameEntity> entities2;
	loadNameEntities(entities2, test_path, "NONE");
	cout << "#enties2: " << entities2.size() << endl;

	neMgr.loadModels();
	neMgr.predict(entities2);

	int posCount=0;
	for (size_t i=0; i<entities2.size(); ++i)
	{
//		cout << i+1 << "\t";
		testOut<<i+1 << "\t";
		string cur="", pre="", suc="";

		entities2[i].cur.convertString(cur, UString::UTF_8);

//		cout << cur << "\t";
		testOut<<cur << "\t";

		std::vector<Label> labels = entities2[i].predictLabels;
		if(labels.size()>0)
		{
			if(labels[0]==entities2[i].tagLabels[0])
				posCount++;
			else
				std::cout<<cur<<entities2[i].tagLabels[0]<<","<<labels[0]<<std::endl;
		}
		else if(entities2[i].tagLabels[0]=="OTHER")
			posCount++;
		else
		{
			std::cout<<cur<<entities2[i].tagLabels[0]<<","<<"NONE"<<std::endl;
		}
		for (size_t j=0; j<labels.size(); ++j)
		{
//			cout << labels[j] << " ";
			testOut<<labels[j] << " ";
		}
//		cout << endl;
		testOut<<std::endl;
	}
	std::cout<<"The classification rate: "<<(float)posCount/entities2.size()<<std::endl;
}

/**extracting word groups */
//    std::string path="/home/jinglei/199801.txt";
//    std::ifstream corpus(path.c_str());
//    std::ofstream adj("noun.txt");
//    std::string line;
//    std::string delimiters="  ";
//    std::set<std::string> dict;
//    while(std::getline(corpus, line))
//    {
//    	if(line.length()>0)
//    	{
//    		std::vector<std::string> tokens;
//    		tokenize(line, tokens, delimiters);
//    		for(size_t i=0;i<tokens.size();i++)
//    		{
//    			std::string strWord;
//    			std::string strLabel;
//    			wiselib::UString uToken(tokens[i], wiselib::UString::GB2312);
//    			std::string utfToken;
//    			uToken.convertString(utfToken, wiselib::UString::UTF_8);
//    			if(utfToken[utfToken.length()-1]==' ')
//    				utfToken=utfToken.substr(0, utfToken.length()-1);
//    			int pos=utfToken.find("/");
//    			if(pos!=std::string::npos)
//    			{
////    				if(utfToken.find("nr")==string::npos)
////    				{
//    				strWord=utfToken.substr(0, pos);
//    				strLabel=utfToken.substr(pos+1,utfToken.length()-pos-1);
//    				if(strLabel=="n")
//    				{
//                        if(dict.find(strWord)==dict.end())
//                        {
//                        	dict.insert(strWord);
//                        }
//
//    				}
////    				}
//    			}
//
//    		}
//    	}
//    }
//
//    std::set<std::string>::iterator iter=dict.begin();
//    for(;iter!=dict.end();iter++)
//    {
//    	adj<<*iter<<std::endl;
//    }

/* extracting patterns _______*/
//        std::string path="/home/jinglei/199801.txt";
//	    std::ifstream corpus(path.c_str());
//	    std::ofstream adj("pattern.txt");
//	    std::string line;
//	    std::string delimiters="  ";
//	    std::map<std::string, int> dict;
//
//	    while(std::getline(corpus, line))
//	    {
//	    	if(line.length()>0)
//	    	{
//	    		std::vector<std::string> tokens;
//	    		tokenize(line, tokens, delimiters);
//	    		typedef std::pair<std::string, std::string> word_label_type;
//	    		std::vector<word_label_type> vecWordLabel;
//	    		for(size_t i=0;i<tokens.size();i++)
//	    		{
//	    			std::string strWord;
//	    			std::string strLabel;
//	    			wiselib::UString uToken(tokens[i], wiselib::UString::GB2312);
//	    			std::string utfToken;
//	    			uToken.convertString(utfToken, wiselib::UString::UTF_8);
//	    			if(utfToken[utfToken.length()-1]==' ')
//	    				utfToken=utfToken.substr(0, utfToken.length()-1);
//	    			int pos=utfToken.find("/");
//	    			if(pos!=std::string::npos)
//	    			{
//	    				strWord=utfToken.substr(0, pos);
//	    				if(strWord.find("[")==0)
//	    					strWord=strWord.substr(1,strWord.length()-1);
//	    				if(strWord.find("]")==strWord.length()-1)
//	    					strWord=strWord.substr(0,strWord.length()-1);
//	    				strLabel=utfToken.substr(pos+1,utfToken.length()-pos-1);
////	    				std::cout<<strWord<<","<<strLabel<<std::endl;
//	    				vecWordLabel.push_back(std::make_pair(strWord, strLabel));
//	    			}
//
//	    		}
//
//	    		for(size_t i=0;i<vecWordLabel.size();i++)
//	    		{
//	    			if(vecWordLabel[i].second=="nr"&&i<vecWordLabel.size()-1)
////	    			if(vecWordLabel[i].second=="nr"&&i>0)
//	    			{
//    				     wiselib::UString ustrTemp(vecWordLabel[i+1].first, wiselib::UString::UTF_8);
//                        if(ustrTemp.length()==1&&vecWordLabel[i+1].second!="nr")
//                        {
////     	    				std::cout<<vecWordLabel[i-1].first.length()<<std::endl;
//
////                        	std::cout<<"here"<<std::endl;
//
//                        	std::map<std::string, int>::iterator it=dict.find(vecWordLabel[i+1].first);
//                        	if(it!=dict.end())
//                        	{
//                        		it->second++;
//                        	}
//                        	else
//                        	{
//                        		dict.insert(std::make_pair(vecWordLabel[i+1].first, 1) );
//                        	}
//
//                        }
//	    			}
//
//	    		}
//	    	}
//	    }
//
//	    std::vector<std::pair<int, std::string> > vecOut;
//	    vecOut.reserve(dict.size());
//	    std::map<std::string, int>::iterator iter=dict.begin();
//	    for(;iter!=dict.end();iter++)
//	    {
//	    	vecOut.push_back(std::make_pair(iter->second, iter->first));
//	    }
//	    std::sort(vecOut.begin(), vecOut.end(), std::greater<std::pair<int, std::string> >());
//	    for(size_t i=0;i<vecOut.size();i++)
//	    {
//	    	adj<<vecOut[i].second<<" "<<vecOut[i].first<<std::endl;
//	    }

	/** select the words to output*/
//	typedef std::map<std::string, int> dict_type;
//	dict_type loc_l, loc_r, org_l, org_r, peop_l, peop_r, all_l, all_r;
//	loadDict("pattern_loc_l.txt", loc_l);
//	loadDict("pattern_org_l.txt", org_l);
//    loadDict("pattern_peop_l.txt", peop_l);
//    ofstream outf("p_all.txt");
//    for(dict_type::iterator it=loc_l.begin();it!=loc_l.end();it++)
//    {
////    	if(peop_l.find(it->first)==peop_l.end()&&org_l.find(it->first)==org_l.end())
////    		outf<<it->first<<" "<<it->second<<std::endl;
//    	if(all_l.find(it->first)==all_l.end())
//    	{
//    		all_l.insert(std::make_pair(it->first, 1));
//    	}
//    }
//    for(dict_type::iterator it=org_l.begin();it!=org_l.end();it++)
//    {
//    	if(all_l.find(it->first)==all_l.end())
//    	{
//    		all_l.insert(std::make_pair(it->first, 1));
//    	}
//    }
//    for(dict_type::iterator it=peop_l.begin();it!=peop_l.end();it++)
//    {
//    	if(all_l.find(it->first)==all_l.end())
//    	{
//    		all_l.insert(std::make_pair(it->first, 1));
//    	}
//    }
//
//    for(dict_type::iterator it=all_l.begin();it!=all_l.end();it++)
//    {
//    	outf<<it->first<<std::endl;
//    }
//	std::vector<NameEntity> entities;
//	loadNameEntities(entities, "peop.txt", "NONE");

bool isEntityLabel(const std::string& strLabel)
{
	if(strLabel=="PER"||strLabel=="LOC"||strLabel=="ORG")
//	if(strLabel=="PER")
	{
		return true;
	}
	return false;
}


void processEngCorpus()
{
    std::string path="train.txt";
    std::ifstream corpus(path.c_str());
    std::ofstream trainPattern("train_pattern.txt");
//    std::ofstream trainPattern_org("train_pattern_org.txt");
//    std::ofstream trainPattern_peop("train_pattern_peop.txt");
//    std::ofstream trainPattern_loc("train_pattern_loc.txt");
//    std::ofstream trainPattern("train_pattern_peop.txt");
    std::string line;
    std::string delimiters=" ";
    std::string trim="_";

    typedef std::map<std::string, NameEntityContextType> EntityListType;
    EntityListType entities;
    std::string lastEntity="";
    std::string lastLabel="";
    std::string lastContext="";
    std::string lastWord="";
    while(std::getline(corpus, line))
    {
    	if(line.length()>0)
    	{
    		std::vector<std::string> tokens;
    		tokenize(line, tokens, delimiters);
    		if(tokens.size()!=3)
    			continue;
    		{
//    			std::cout<<tokens[0]<<" "<<tokens[1]<<" "<<tokens[2]<<std::endl;
    			std::string strWord=tokens[0];
    			std::string strLabel=tokens[2];
    			if(strLabel!=lastLabel)
    			{
    				if(isEntityLabel(lastLabel))
    				{
    					trimRight(lastEntity, trim.c_str());
                        EntityListType::iterator it=entities.find(lastEntity);
                        if(it==entities.end())
                        {
                        	NameEntityContextType context;
                        	if(strWord.length()>0)
                                context.suc_.insert(strWord);
                        	if(lastContext.length()>0)
                                context.pre_.insert(lastContext);
                            entities.insert(std::make_pair(lastEntity, context));
                         }
                        else
                        {
                        	if(strWord.length()>0)
                        	    it->second.suc_.insert(strWord);
                        	if(lastContext.length()>0)
                        	    it->second.pre_.insert(lastContext);
                        }

                        lastEntity="";
                        lastLabel="";
                        lastContext="";
    				}
    				if(isEntityLabel(strLabel))
    				{
    					lastEntity=strWord+"_";
    					lastLabel=strLabel;
    					lastContext=lastWord;
    				}

    			}
    			else
    			{
    				if(isEntityLabel(strLabel))
    				{
    					lastEntity=lastEntity+strWord+"_";
    				}
    			}
    			lastWord=strWord;
     		}
    	}
    }//end while

    EntityListType::iterator it=entities.begin();
    for(;it!=entities.end();it++)
    {
        trainPattern<<it->first<<" ";
        if(it->second.pre_.size()>0)
        {
        	std::set<std::string>::iterator iterLeft=it->second.pre_.begin();
            for(;iterLeft!=it->second.pre_.end();iterLeft++)
            {
            	if(*iterLeft!="_")
            	    trainPattern<<"L_"<<*iterLeft<<"_x ";
            }
        }
        if(it->second.suc_.size()>0)
        {
        	std::set<std::string>::iterator iterRight=it->second.suc_.begin();
            for(;iterRight!=it->second.suc_.end();iterRight++)
            {
            	if(*iterRight!="_")
            	    trainPattern<<"R_"<<*iterRight<<"_x ";
            }
        }

        trainPattern<<std::endl;
    }

}

int main()
{
//	processEngCorpus();
	testNec();
}

