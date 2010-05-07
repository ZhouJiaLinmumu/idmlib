/*
 * NameEntityUtil.h
 *
 *  Created on: Apr 8, 2010
 *      Author: eric
 */

#ifndef DM_NAMEENTITYUTIL_H_
#define DM_NAMEENTITYUTIL_H_

#include <ml/ClassificationDataUtil.h>
#include "util/Vectors.h"
#include "NameEntity.h"
#include <idmlib/util/IntIdMgr.h>
#include "NameEntityDict.h"

using namespace idmlib;

namespace ml
{

	template<>
	void ClassificationDataUtil<NameEntity>::transform(NameEntity& entity, ml::Schema& schema, ml::Instance& inst)
	{
//		IdManager* idMgr = IdMgrFactory::getIdManager();

		UString cur = entity.cur;
		std::vector<UString> suc = entity.suc;
		std::vector<UString> pre = entity.pre;

		// current character sequence
		size_t curLength = cur.length();
		UString cur_b, b_cur_b, cur_e, cur_a, b_cur_a, b_cur_e, t_cur_a, t_cur_e, cur_l, left, right;
		UString utag_curb("_UCB", UString::UTF_8); // the begin character of current character sequence
		UString btag_curb("_BCB", UString::UTF_8); // the begin bigram of current character sequence
		UString utag_cure("_UCE", UString::UTF_8); // the end character of current character sequence
		UString utag_cura("_UCA", UString::UTF_8); // all characters of current character sequence
		UString btag_cura("_BCA", UString::UTF_8); // all bigrams of current sequence
//		UString ttag_cura("_TCA", UString::UTF_8); // all trigrams of current sequence
		UString btag_cure("_BCE", UString::UTF_8); // the end bigram
		UString ttag_cure("_TCE", UString::UTF_8); // the end trigram
		UString utag_curl("_UL", UString::UTF_8); // length
		UString btag_curn("_BN", UString::UTF_8); // has noise
		UString btag_curo("_BO", UString::UTF_8); // has other bigram
		UString utag_left("_LEFT", UString::UTF_8); // has left context
		UString utag_right("_RIGHT", UString::UTF_8); // has left context

		std::vector<UString> f_cur_u_b;
		std::vector<UString> f_cur_b_b;
		std::vector<UString> f_cur_u_a;
		std::vector<UString> f_cur_b_a;
//		std::vector<UString> f_cur_t_a;
		std::vector<UString> f_cur_b_e;
		std::vector<UString> f_cur_t_e;
		std::vector<UString> f_cur_u_e;
		std::vector<UString> f_cur_b_n;
		std::vector<UString> f_cur_b_o;
		std::vector<UString> f_cur_l;
		std::vector<UString> f_left;
		std::vector<UString> f_right;
		std::vector<UString> f_all;

		std::vector<ml::AttrID> id_cur_u_b;
		std::vector<ml::AttrID> id_cur_b_b;
		std::vector<ml::AttrID> id_cur_u_a;
		std::vector<ml::AttrID> id_cur_b_a;
//		std::vector<ml::AttrID> id_cur_t_a;
		std::vector<ml::AttrID> id_cur_b_e;
		std::vector<ml::AttrID> id_cur_t_e;
		std::vector<ml::AttrID> id_cur_u_e;
		std::vector<ml::AttrID> id_cur_b_n;
		std::vector<ml::AttrID> id_cur_b_o;
		std::vector<ml::AttrID> id_cur_l;
		std::vector<ml::AttrID> id_left;
		std::vector<ml::AttrID> id_right;


		double w_cur_u_b = 3;
		double w_cur_b_b = 4;
		double w_cur_u_a = 2;
		double w_cur_b_a = 2;
//		double w_cur_t_a = 2;
		double w_cur_b_e = 16;
		double w_cur_t_e = 16;
		double w_cur_u_e = 8;
		double w_cur_b_n = 0.5;
		double w_cur_b_o = 1.5;
		double w_left = 3;
		double w_right = 3;
//		double w_cur_l =0.4;

		if (curLength >= 1)
		{
			cur.substr(cur_b, 0, 1);
			cur_b.append(utag_curb);

			cur.substr(cur_e, curLength-1, 1);
			cur_e.append(utag_cure);

			f_cur_u_b.push_back(cur_b);	// the first unigram of current sequence
			f_all.push_back(cur_b);

			f_cur_u_e.push_back(cur_e);  // the last unigram of current sequence
			f_all.push_back(cur_e);

			IntIdMgr::getTermIdListByTermStringList(f_cur_u_b, id_cur_u_b);
			for (size_t i=0; i<id_cur_u_b.size(); ++i)
			{
				inst.x.set(id_cur_u_b[i], w_cur_u_b);
				schema.setAttr(id_cur_u_b[i], 1);
			}

			IntIdMgr::getTermIdListByTermStringList(f_cur_u_e, id_cur_u_e);
			for (size_t i=0; i<id_cur_u_e.size(); ++i)
			{
				inst.x.set(id_cur_u_e[i], w_cur_u_e);
				schema.setAttr(id_cur_u_e[i], 1);
			}

			for (size_t i=0; i<curLength; ++i)
			{
				cur.substr(cur_a, i, 1);
				cur_a.append(utag_cura);

				f_cur_u_a.push_back(cur_a);	  // unigram of current sequence
				f_all.push_back(cur_a);
			}

			IntIdMgr::getTermIdListByTermStringList(f_cur_u_a, id_cur_u_a);
			for (size_t i=0; i<id_cur_u_a.size(); ++i)
			{
				inst.x.set(id_cur_u_a[i], w_cur_u_a);
				schema.setAttr(id_cur_u_a[i], 1);
			}

		}

		string b_cur_e_str;
		string t_cur_e_str;

		if (curLength > 1)
		{

			cur.substr(b_cur_b, 0, 2);  // the first bigram of current sequence
			b_cur_b.append(btag_curb);
			f_cur_b_b.push_back(b_cur_b);
			f_all.push_back(b_cur_b);

			cur.substr(b_cur_e, curLength-2, 2);
			b_cur_e.convertString(b_cur_e_str, UString::UTF_8);
			b_cur_e.append(btag_cure);


			f_cur_b_e.push_back(b_cur_e);  // the last bigram of current sequence
			f_all.push_back(b_cur_e);

			IntIdMgr::getTermIdListByTermStringList(f_cur_b_b, id_cur_b_b);
			for (size_t i=0; i<id_cur_b_b.size(); ++i)
			{
				inst.x.set(id_cur_b_b[i], w_cur_b_b);
				schema.setAttr(id_cur_b_b[i], 1);
			}

			IntIdMgr::getTermIdListByTermStringList(f_cur_b_e, id_cur_b_e);
			for (size_t i=0; i<id_cur_b_e.size(); ++i)
			{
				inst.x.set(id_cur_b_e[i], w_cur_b_e);
				schema.setAttr(id_cur_b_e[i], 1);
			}

			bool hasNoiseBigram=false;
			bool hasOtherBigram=false;
			for (size_t i=0; i<curLength-1; ++i)
			{
				cur.substr(b_cur_a, i, 2);
				std::string strBigram;
				b_cur_a.convertString(strBigram, wiselib::UString::UTF_8);
				b_cur_a.append(btag_cura);
				f_cur_b_a.push_back(b_cur_a);  // bigram of current sequence
				f_all.push_back(b_cur_a);
				if(NameEntityDict::isKownNoise(strBigram))
					hasNoiseBigram=true;
				else if(NameEntityDict::isNoun(strBigram))
					hasOtherBigram=true;
			}
			if(hasNoiseBigram)
			{
				f_cur_b_n.push_back(btag_curn);
				f_all.push_back(btag_curn);
			}

			if(hasOtherBigram)
			{
				f_cur_b_o.push_back(btag_curo);
				f_all.push_back(btag_curo);
			}

			IntIdMgr::getTermIdListByTermStringList(f_cur_b_a, id_cur_b_a);

			for (size_t i=0; i<id_cur_b_a.size(); ++i)
			{
				inst.x.set(id_cur_b_a[i], w_cur_b_a);
				schema.setAttr(id_cur_b_a[i], 1);
			}

			IntIdMgr::getTermIdListByTermStringList(f_cur_b_n, id_cur_b_n);

			for (size_t i=0; i<id_cur_b_n.size(); ++i)
			{
				inst.x.set(id_cur_b_n[i], w_cur_b_n);
				schema.setAttr(id_cur_b_n[i], 1);
			}

			IntIdMgr::getTermIdListByTermStringList(f_cur_b_o, id_cur_b_o);

			for (size_t i=0; i<id_cur_b_o.size(); ++i)
			{
				inst.x.set(id_cur_b_o[i], w_cur_b_o);
				schema.setAttr(id_cur_b_o[i], 1);
			}


		}

		if (curLength > 2)
		{
			cur.substr(t_cur_e, curLength-3, 3);
			t_cur_e.convertString(t_cur_e_str, UString::UTF_8);
			t_cur_e.append(ttag_cure);

			f_cur_t_e.push_back(t_cur_e);  // the last trigram of current sequence
			f_all.push_back(t_cur_e);

			IntIdMgr::getTermIdListByTermStringList(f_cur_t_e, id_cur_t_e);
			for (size_t i=0; i<id_cur_t_e.size(); ++i)
			{
				inst.x.set(id_cur_t_e[i], w_cur_t_e);
				schema.setAttr(id_cur_t_e[i], 1);
			}

//			for (size_t i=0; i<curLength-2; ++i)
//			{
//				cur.substr(t_cur_a, i, 3);
//				t_cur_a.append(ttag_cura);
//				f_cur_t_a.push_back(t_cur_a);  // trigram of current sequence
//				f_all.push_back(t_cur_a);
//
//			}
//
//			idMgr->getTermIdListByTermStringList(f_cur_t_a, id_cur_t_a);
//
//			for (size_t i=0; i<id_cur_t_a.size(); ++i)
//			{
//				inst.x.set(id_cur_t_a[i], w_cur_t_a);
//				schema.setAttr(id_cur_t_a[i], 1);
//			}
		}

		std::vector<UString> f_cur_loc;
		std::vector<ml::AttrID> id_cur_loc;
		double w_cur_loc = 10;

		std::vector<UString> f_cur_loc2;
		std::vector<ml::AttrID> id_cur_loc2;
		double w_cur_loc2 = 12;

		std::vector<UString> f_cur_loc3;
		std::vector<ml::AttrID> id_cur_loc3;
		double w_cur_loc3 = 12;


		// whether is locaction suffix
		UString utag_cure_loc("_UCEL", UString::UTF_8);
		UString btag_cure_loc("_BCEL", UString::UTF_8);
		UString ttag_cure_loc("_TCEL", UString::UTF_8);

		if (curLength >= 1)
		{
			UString cur_e;
			cur.substr(cur_e, curLength-1, 1);
			string loc;
			cur_e.convertString(loc, UString::UTF_8);
			if(NameEntityDict::isLocSuffix(loc))
			{
				f_cur_loc.push_back(utag_cure_loc);
				f_all.push_back(utag_cure_loc);
				IntIdMgr::getTermIdListByTermStringList(f_cur_loc, id_cur_loc);

				for (size_t i=0; i<id_cur_loc.size(); ++i)
				{
					inst.x.set(id_cur_loc[i], w_cur_loc);
					schema.setAttr(id_cur_loc[i], 1);
				}
			}

			if (curLength >= 2)
			{
				if (NameEntityDict::isLocSuffix(b_cur_e_str))
				{
					f_cur_loc2.push_back(btag_cure_loc);
					f_all.push_back(btag_cure_loc);
					IntIdMgr::getTermIdListByTermStringList(f_cur_loc2, id_cur_loc2);

					for (size_t i =0; i<id_cur_loc2.size(); ++i)
					{
						inst.x.set(id_cur_loc2[i], w_cur_loc2);
						schema.setAttr(id_cur_loc2[i], 1);
					}
				}
			}

			if (curLength >= 3)
			{
				if (NameEntityDict::isLocSuffix(t_cur_e_str))
				{
					f_cur_loc3.push_back(ttag_cure_loc);
					f_all.push_back(ttag_cure_loc);
					IntIdMgr::getTermIdListByTermStringList(f_cur_loc3, id_cur_loc3);

					for (size_t i =0; i<id_cur_loc3.size(); ++i)
					{
						inst.x.set(id_cur_loc3[i], w_cur_loc3);
						schema.setAttr(id_cur_loc3[i], 1);
					}
				}
			}


		}


	//
		std::vector<UString> f_cur_org;
		std::vector<ml::AttrID> id_cur_org;
		double w_cur_org = 10;

		std::vector<UString> f_cur_org2;
		std::vector<ml::AttrID> id_cur_org2;
		double w_cur_org2 = 12;

		std::vector<UString> f_cur_org3;
		std::vector<ml::AttrID> id_cur_org3;
		double w_cur_org3 = 12;

		// whether is org suffix
		UString utag_cure_org("_UCEO", UString::UTF_8);
		UString btag_cure_org("_BCEO", UString::UTF_8);
		UString ttag_cure_org("_TCEO", UString::UTF_8);

		if (curLength >= 1)
		{
			UString cur_e;
			cur.substr(cur_e, curLength-1, 1);
			string org;
			cur_e.convertString(org, UString::UTF_8);
			if(NameEntityDict::isOrgSuffix(org))
			{
				f_cur_org.push_back(utag_cure_org);
				f_all.push_back(utag_cure_org);
				IntIdMgr::getTermIdListByTermStringList(f_cur_org, id_cur_org);
				for (size_t i=0; i<id_cur_org.size(); ++i)
				{
					inst.x.set(id_cur_org[i], w_cur_org);
					schema.setAttr(id_cur_org[i], 1);
				}
			}

			if (curLength >= 2)
			{
				if (NameEntityDict::isOrgSuffix(b_cur_e_str))
				{
					f_cur_org2.push_back(btag_cure_org);
					f_all.push_back(btag_cure_org);
					IntIdMgr::getTermIdListByTermStringList(f_cur_org2, id_cur_org2);

					for (size_t i =0; i<id_cur_org2.size(); ++i)
					{
						inst.x.set(id_cur_org2[i], w_cur_org2);
						schema.setAttr(id_cur_org2[i], 1);
					}
				}
			}

			if (curLength >= 3)
			{
				if (NameEntityDict::isOrgSuffix(t_cur_e_str))
				{
					f_cur_org3.push_back(ttag_cure_org);
					f_all.push_back(ttag_cure_org);
					IntIdMgr::getTermIdListByTermStringList(f_cur_org3, id_cur_org3);

					for (size_t i =0; i<id_cur_org3.size(); ++i)
					{
						inst.x.set(id_cur_org3[i], w_cur_org3);
						schema.setAttr(id_cur_org3[i], 1);
					}
				}
			}
		}

		std::vector<UString> f_cur_peop;
		std::vector<ml::AttrID> id_cur_peop;
		double w_cur_peop = 4;

		std::vector<UString> f_cur_peop2;
		std::vector<ml::AttrID> id_cur_peop2;
		double w_cur_peop2 = 12;

		std::vector<UString> f_cur_peop3;
		std::vector<ml::AttrID> id_cur_peop3;
		double w_cur_peop3 = 12;

		// whether is org suffix
		UString utag_cure_peop("_UCEP", UString::UTF_8);
		UString btag_cure_peop("_BCEP", UString::UTF_8);
		UString ttag_cure_peop("_TCEP", UString::UTF_8);

		if (curLength >= 1)
		{
			UString cur_e;
			cur.substr(cur_e, curLength-1, 1);
			string peop;
			cur_e.convertString(peop, UString::UTF_8);
			if(NameEntityDict::isPeopSuffix(peop))
			{
				f_cur_peop.push_back(utag_cure_peop);
				f_all.push_back(utag_cure_peop);
				IntIdMgr::getTermIdListByTermStringList(f_cur_peop, id_cur_peop);

				for (size_t i=0; i<id_cur_peop.size(); ++i)
				{
					inst.x.set(id_cur_peop[i], w_cur_peop);
					schema.setAttr(id_cur_peop[i], 1);
				}
			}

			if (curLength >= 2)
			{
				if (NameEntityDict::isPeopSuffix(b_cur_e_str))
				{
					f_cur_peop2.push_back(btag_cure_peop);
					f_all.push_back(btag_cure_peop);
					IntIdMgr::getTermIdListByTermStringList(f_cur_peop2, id_cur_peop2);

					for (size_t i =0; i<id_cur_peop2.size(); ++i)
					{
						inst.x.set(id_cur_peop2[i], w_cur_peop2);
						schema.setAttr(id_cur_peop2[i], 1);
					}
				}
			}

			if (curLength >= 3)
			{
				if (NameEntityDict::isPeopSuffix(t_cur_e_str))
				{
					f_cur_peop3.push_back(ttag_cure_peop);
					f_all.push_back(ttag_cure_peop);
					IntIdMgr::getTermIdListByTermStringList(f_cur_peop3, id_cur_peop3);

					for (size_t i =0; i<id_cur_peop3.size(); ++i)
					{
						inst.x.set(id_cur_peop3[i], w_cur_peop3);
						schema.setAttr(id_cur_peop3[i], 1);
					}
				}
			}

		}

	// whether is name prefix
		std::vector<UString> f_cur_surname;
		std::vector<ml::AttrID> id_cur_surname;
		double w_cur_surname = 6;
		UString utag_curb_name("_UCBN", UString::UTF_8);
		if (curLength == 3||curLength ==2)
		{
			UString cur_b;
			cur.substr(cur_b, 0, 1);
			string name;
			cur_b.convertString(name, UString::UTF_8);
			if(NameEntityDict::isNamePrefix(name))
			{
				f_cur_surname.push_back(utag_curb_name);
				f_all.push_back(utag_curb_name);
			}
		}

		IntIdMgr::getTermIdListByTermStringList(f_cur_surname, id_cur_surname);

		for (size_t i=0; i<id_cur_surname.size(); ++i)
		{
			inst.x.set(id_cur_surname[i], w_cur_surname);
			schema.setAttr(id_cur_surname[i], 1);
		}



//		UString lenStr;
//		// current sequence length
//		if (curLength == 2)
//		{
//			UString ustr("2", UString::UTF_8);
//			lenStr = ustr;
//		} else  if (curLength == 3)
//		{
//			UString ustr("3", UString::UTF_8);
//			lenStr = ustr;
//		} else
//		{
//			UString ustr("L", UString::UTF_8);
//			lenStr = ustr;
//		}
//
//		cur_l.append(lenStr);
//		cur_l.append(utag_curl);
//		f_cur_l.push_back(cur_l);
//		f_all.push_back(cur_l);
//
//		IntIdMgr::getTermIdListByTermStringList(f_cur_l, id_cur_l);
//
//		for (size_t i =0; i<id_cur_l.size(); ++i)
//		{
//			inst.x.set(id_cur_l[i], w_cur_l);
//			schema.setAttr(id_cur_l[i], 1);
//		}

		UString peopTag("_PEOP", wiselib::UString::UTF_8);
		UString locTag("_LOC", wiselib::UString::UTF_8);
		UString orgTag("_ORG", wiselib::UString::UTF_8);
		if(pre.size()>0)
		{
			for(size_t i=0;i<pre.size();i++)
			{
				UString preChar=pre[i];
				preChar.append(utag_left);
				std::string strItem;
				pre[i].convertString(strItem, wiselib::UString::UTF_8);
				if(NameEntityDict::isPeopLeft(strItem))
				{
                   	preChar.append(peopTag);
					f_left.push_back(preChar);
					f_all.push_back(preChar);
				}
				else if(NameEntityDict::isLocLeft(strItem))
				{
                   	preChar.append(locTag);
					f_left.push_back(preChar);
					f_all.push_back(preChar);
				}
				else if(NameEntityDict::isOrgLeft(strItem))
				{
                   	preChar.append(orgTag);
					f_left.push_back(preChar);
					f_all.push_back(preChar);
				}
			}

			IntIdMgr::getTermIdListByTermStringList(f_left, id_left);
			for (size_t i =0; i<id_left.size(); ++i)
			{
				inst.x.set(id_left[i], w_left);
				schema.setAttr(id_left[i], 1);
			}
		}
		if(suc.size()>0)
		{
			for(size_t i=0;i<suc.size();i++)
			{
				UString preChar=suc[i];
				preChar.append(utag_right);
				std::string strItem;
				suc[i].convertString(strItem, wiselib::UString::UTF_8);
				if(NameEntityDict::isPeopRight(strItem))
				{
                   	preChar.append(peopTag);
					f_right.push_back(preChar);
					f_all.push_back(preChar);
				}
				else if(NameEntityDict::isLocRight(strItem))
				{
                   	preChar.append(locTag);
					f_right.push_back(preChar);
					f_all.push_back(preChar);
				}
				else if(NameEntityDict::isOrgRight(strItem))
				{
                   	preChar.append(orgTag);
					f_right.push_back(preChar);
					f_all.push_back(preChar);
				}
			}

			IntIdMgr::getTermIdListByTermStringList(f_right, id_right);
			for (size_t i =0; i<id_right.size(); ++i)
			{
				inst.x.set(id_right[i], w_right);
				schema.setAttr(id_right[i], 1);
			}
		}

	//	size_t window = 3;

	//	// previous character sequence
	//	size_t preLength = pre.length();
	//
	//	UString pre1, pre12, pre_a, b_pre_a;
	//	UString utag_pre1("_UP1", UString::UTF_8); // C-1
	//	UString utag_pre12("_UP12", UString::UTF_8); // C-12
	//	UString utag_pre_a("_UPA", UString::UTF_8); // C-UA
	//	UString btag_pre_a("_BPA", UString::UTF_8); // C-BA
	//
	//	if (preLength >= 1)
	//	{
	//		pre.substr(pre1, preLength-1, 1);
	//		pre1.append(utag_pre1);
	//
	//		features.push_back(pre1);
	//
	//		size_t minWin = window;
	//		if (window > preLength)
	//		{
	//			minWin = preLength;
	//		}
	//		for (size_t i=0; i<minWin; ++i)
	//		{
	//			pre.substr(pre_a, i, 1);
	//			pre_a.append(utag_pre_a);
	//
	//			features.push_back(pre_a);
	//		}
	//	}
	//
	//	if (preLength >= 2)
	//	{
	//		pre.substr(pre12, preLength-2, 2);
	//		pre12.append(utag_pre12);
	//
	//		features.push_back(pre12);
	//
	//		for (size_t i=0; i<preLength-1; ++i)
	//		{
	//			pre.substr(b_pre_a, i, 2);
	//			b_pre_a.append(btag_pre_a);
	//
	//			features.push_back(b_pre_a);
	//		}
	//	}
	//
	////	// successor character sequence
	//	size_t sucLength = suc.length();
	//	UString suc1, suc12, suc_a, b_suc_a;
	//	UString utag_suc1("_US1", UString::UTF_8); // C1
	//	UString utag_suc12("_US12", UString::UTF_8); // C12
	//	UString utag_suc_a("_USA", UString::UTF_8); // C+UA
	//	UString btag_suc_a("_BSA", UString::UTF_8); // C+BA
	//
	//	if (sucLength >= 1)
	//	{
	//		suc.substr(suc1, 0, 1);
	//		suc1.append(utag_suc1);
	//
	//		features.push_back(suc1);
	//
	//		size_t minWin = window;
	//		if (window > sucLength)
	//		{
	//			minWin = sucLength;
	//		}
	//		for (size_t i=0; i<minWin; ++i)
	//		{
	//			suc.substr(suc_a, i, 1);
	//			suc_a.append(utag_suc_a);
	//
	//			features.push_back(suc_a);
	//		}
	//	}
	//
	//	if (sucLength >= 2)
	//	{
	//		suc.substr(suc12, 0, 2);
	//		suc12.append(utag_suc12);
	//
	//		features.push_back(suc12);
	//
	//		for (size_t i=0; i<sucLength-1; ++i)
	//		{
	//			suc.substr(b_suc_a, i, 2);
	//			b_suc_a.append(btag_suc_a);
	//
	//			features.push_back(b_suc_a);
	//		}
	//	}

		//
	//	UString utag_pre1_u("_UP1_U", UString::UTF_8);
	//	UString utag_pre1_x("_UP1_X", UString::UTF_8);
	//	UString utag_pre1_p("_UP1_P", UString::UTF_8);
	//	UString utag_pre1_q("_UP1_Q", UString::UTF_8);
	//	UString utag_pre1_c("_UP1_C", UString::UTF_8);
	//	UString utag_pre1_d("_UP1_D", UString::UTF_8);
	//
	//	UString utag_suc1_u("_US1_U", UString::UTF_8);
	//	UString utag_suc1_x("_US1_X", UString::UTF_8);
	//	UString utag_suc1_p("_US1_P", UString::UTF_8);
	//	UString utag_suc1_q("_US1_Q", UString::UTF_8);
	//	UString utag_suc1_c("_US1_C", UString::UTF_8);
	//	UString utag_suc1_d("_US1_D", UString::UTF_8);
	//
	//
	//	if (preLength >= 1)
	//	{
	//		UString pre1;
	//		pre.substr(pre1, preLength-1, 1);
	//		string ch;
	//		pre1.convertString(ch, UString::UTF_8);
	//		if (NameEntityDict::isU(ch))
	//		{
	//			features.push_back(utag_pre1_u);
	//		} else if (NameEntityDict::isC(ch))
	//		{
	//			features.push_back(utag_pre1_c);
	//		} else if (NameEntityDict::isD(ch))
	//		{
	//			features.push_back(utag_pre1_d);
	//		} else if (NameEntityDict::isQ(ch))
	//		{
	//			features.push_back(utag_pre1_q);
	//		} else if (NameEntityDict::isP(ch))
	//		{
	//			features.push_back(utag_pre1_p);
	//		} else if (NameEntityDict::isX(ch))
	//		{
	//			features.push_back(utag_pre1_x);
	//		}
	//
	//	}
	//
	//	if (sucLength >= 1)
	//	{
	//		UString suc1;
	//		suc.substr(suc1, 0, 1);
	//		string ch;
	//		suc1.convertString(ch, UString::UTF_8);
	//		if (NameEntityDict::isU(ch))
	//		{
	//			features.push_back(utag_suc1_u);
	//		} else if (NameEntityDict::isC(ch))
	//		{
	//			features.push_back(utag_suc1_c);
	//		} else if (NameEntityDict::isD(ch))
	//		{
	//			features.push_back(utag_suc1_d);
	//		} else if (NameEntityDict::isQ(ch))
	//		{
	//			features.push_back(utag_suc1_q);
	//		} else if (NameEntityDict::isP(ch))
	//		{
	//			features.push_back(utag_suc1_p);
	//		} else if (NameEntityDict::isX(ch))
	//		{
	//			features.push_back(utag_suc1_x);
	//		}
	//	}

//		for (size_t i=0; i<f_all.size(); ++i)
//		{
//			string feature;
//			f_all[i].convertString(feature, UString::UTF_8);
//			cout << feature << endl;
//		}
//		cout << endl;

	//	cout << "#features:" << features.size() << endl;
	//	idMgr->getTermIdListByTermStringList(features, featureIds);
	//
	//	for (size_t i=0; i<featureIds.size(); ++i)
	//	{
	//		inst.x.set(featureIds[i], 1);
	//		schema.setAttr(featureIds[i], 1);
	//	}

	}


	template<>
	void ClassificationDataUtil<NameEntity>::transform(std::vector<NameEntity>& entities, ml::InstanceBag& instBag)
	{
		std::vector<UString> features;
		std::vector<ml::AttrID> featureIds;
		std::vector<NameEntity>::const_iterator it;
		ml::Schema schema;
		for (it = entities.begin(); it != entities.end(); ++it)
		{
			NameEntity entity = (*it);
			ml::Instance inst;
			transform(entity, schema, inst);
			instBag.addInst(inst);
			std::vector<Label> labels = (*it).tagLabels;
			for (std::vector<Label>::const_iterator it = labels.begin();
							it != labels.end(); ++it)
			{
				Label label = *it;
				if (std::find(labels.begin(), labels.end(), label) == labels.end())
				{
					labels.push_back(label);
					schema.addLabel(label);
				}
			}
		}

		instBag.setSchema(schema);
	}
}

#endif /* NAMEENTITYUTIL_H_ */
