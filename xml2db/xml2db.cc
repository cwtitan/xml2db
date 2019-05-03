/* vim: set sts=4 sw=4: */
#define BOOST_SYSTEM_NO_DEPRECATED 1
#define BOOST_FILESYSTEM_VERSION 3

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <deque>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <cmath>

#include <expat.h>

#include "binfile.h"
#include "const.h"
#include "filename.h"
#include "pigg.h"
#include "pstring.h"
#include "recordtypes.h"

// extern "C" { int ValidateFile(const char *filename); }

namespace fs = boost::filesystem;

using namespace std;
using namespace boost;
using namespace coh;

#define foreach BOOST_FOREACH

#define BADGEBITS_SIZE 290

PString pstr;
Bin<record::AttribName> anames;
Bin<record::PowerCat> powercats;
map<string, record::PowerCat const*> powercatidx;
Bin<record::PowerSet> powersets;
map<string, record::PowerSet const*> powersetidx;
Bin<record::Power> powers;
map<string, record::Power const*> poweridx;
Bin<record::BoostSet> boostsets;
Bin<record::CClass> classes;
map<string, record::CClass const*> classidx;
Bin<record::Origin> origins;
Bin<record::Schedules> schedules;
Bin<record::Experience> experience;
Bin<record::Badge> badges;
map<string, record::Badge const*> badgeidx;
Bin<record::Salvage> salvage;
map<string, record::Salvage const*> salvageidx;
Bin<record::BodyPart> bodyparts;
Bin<record::Costume> costumes;

struct CostumeKey {
    CostumeKey(int bt, string g, string t1, string t2, string f):
	bodytype(bt), geo(g), tex1(t1), tex2(t2), fx(f) {}

    int bodytype;
    string geo;
    string tex1;
    string tex2;
    string fx;

    private:
    CostumeKey(): bodytype(0) {};
};

bool operator< (CostumeKey const& lhs, CostumeKey const& rhs) {
    return ((lhs.bodytype == rhs.bodytype) ?
	    (lhs.geo == rhs.geo) ?
	    (lhs.tex1 == rhs.tex1) ?
	    (lhs.tex2 == rhs.tex2) ?
	    (lhs.fx < rhs.fx) :
	    (lhs.tex2 < rhs.tex2) :
	    (lhs.tex1 < rhs.tex1) :
	    (lhs.geo < rhs.geo) :
	    (lhs.bodytype < rhs.bodytype));
}


struct CostumeLoc {
    CostumeLoc(record::costume::Body const* b,
	    record::costume::Region const* r,
	    record::costume::BoneSet const* bs,
	    record::costume::GeoSet const* gs):
	body(b), region(r), boneset(bs), geoset(gs) {}

    record::costume::Body const* body;
    record::costume::Region const* region;
    record::costume::BoneSet const* boneset;
    record::costume::GeoSet const* geoset;
};

typedef vector<CostumeLoc> CostumeLocList;
typedef map<CostumeKey, CostumeLocList> CostumeIdxMap;

CostumeIdxMap costumeidx;

const char *commontitles[] = {
    "Awesome", "Bold", "Courageous", "Daring", "Extraordinary", "Famous", "Gallant", "Heroic", "Incomparable", "Legendary", "Magnificent", "Outstanding", "Powerful", "Remarkable", "Startling", "Terrific", "Ultimate", "Valiant", "Wonderful",
    "Astonishing", "Battle-Hardened", "Cunning", "Demure", "Ethereal", "Fantastic", "Gargantuan", "Honest", "Impervious", "Lionized", "Nimble", "Overcharged", "Precise", "Resistant", "Speedy", "Tough", "Utilitarian", "Vengeful", "Wanton",
    "Awful", "Brutal", "Conniving", "Dastardly", "Evil", "Fearsome", "Grim", "Heinous", "Infamous", "Killer", "Loathsome", "Malevolent", "Notorious", "Odious", "Potent", "Ruthless", "Sinister", "Terrible", "Unholy", "Villainous", "Wrathful", "Zealous",
    "Angry", "Baneful", "Corrupt", "Dark", "Exiled", "Foul", "Gross", "Hateful", "Innocent", "Low", "Nefarious", "Offensive", "Pernicious", "Revolting", "Salacious", "Terrifying", "Unbounded", "Vengeful", "Weary",
    "Antagonizer", "Bumpbugged", "Chomper", "Cleansweeper", "Defiant", "Downtrodden", "Equalizer", "Giantkiller", "Harbinger", "Hoodlum", "Jazzed", "Jonesy", "Refuser", "Slamcake", "Slammer", "Squeezed", "Statesmasher", "Stickler", "Styled", "Sweetcheek", "Toppler", "Watchdog",
    "Diligent", "Blessed", "Committed", "Determined", "Exemplary", "Faithful", "Grateful", "Honorable", "Intelligent", "Knowledgeable", "Loyal", "Memorable", "Noble", "Observant", "Proud", "Radiant", "Sanguine", "Temperant", "Unflinching", "Victorious", "Watchful", "Zealous",
    "Attendant", "Benevolent", "Confident", "Deserving", "Epitome", "Famous", "Generous", "Hopeful", "Idealistic", "Law-Abiding", "Nationalist", "Orderly", "Prudent", "Receptive", "Sincere", "Trustworthy", "Unequalled", "Valiant", "Wilful"
};

struct CostumePart {
    CostumePart(): slot(0), color1(0), color2(0), color3(0), color4(0), hasfx(false), empty(true) {}

    explicit CostumePart(int s, string p): slot(s), color1(0), color2(0), color3(0), color4(0), hasfx(false), empty(true) {
	stringstream pparse;

	pparse << std::hex << p;
	pparse >> geo;
	if (geo != "none")
	    empty = false;
	pparse >> tex1;
	if (tex1 != "none")
	    empty = false;
	pparse >> tex2;
	if (tex2 != "none")
	    empty = false;

	pparse >> color1;
	pparse >> color2;
	pparse >> color3;
	pparse >> color4;
	if (!(pparse >> fx).fail())
	    hasfx = true;
    }

    int slot;
    string geo;
    string tex1;
    string tex2;
    string fx;
    uint32_t color1;
    uint32_t color2;
    uint32_t color3;
    uint32_t color4;

    string region;
    string bodyset;

    bool hasfx;
    bool empty;
};

struct Costume {
    Costume(): bodytype(0), skincolor(0) {}

    int bodytype;
    int skincolor;
    string scales;
    vector<CostumePart> parts;
};

struct Boost {
    Boost(): empty(true), level(0), boost(0) {}
    explicit Boost(bool mpt): empty(mpt) {}
    Boost(int lv, int bi, string pw): empty(false), level(lv), boost(bi), power(pw) {}

    bool empty;
    int level;
    int boost;
    string power;
};

struct Power {
    Power(): levelbought(0), buildnum(0), disabled(false) {}
    Power(string i, int l, int b, bool d): id(i), levelbought(l),
	buildnum(b), disabled(d) {}

    string id;
    int levelbought;
    int buildnum;
    bool disabled;
    vector<Boost> slots;
};

struct ParseInfo {
    ParseInfo(): inchar(false), counter(0), xp(0), level(0),
    cclass(0), curbuild(0),
    curcostume(0), curpartidx(0),
    bonusrecipe(0), bonussalvage(0), bonusauction(0)
    {
	memset(badgebits, 0, sizeof(badgebits));
    }

    bool inchar;
    vector<string> tagstack;
    string charbuf;

    int counter;
    int regs[8];

    // stuff that needs more info to postprocess at the end
    int xp;
    int level;
    record::CClass const* cclass;
    string alignment;
    string dimension;
    string primaryset;
    string secondaryset;
    vector<Power> powers;
    vector<Boost> enhancements;
    vector<Costume> costumes;
    vector<string> inspirations;
    int curbuild;
    int curcostume;
    int curpartidx;
    int bonusrecipe;
    int bonussalvage;
    int bonusauction;
    map<int, int> colorfreq;
    map<string, int> setlevels;

    unsigned char badgebits[BADGEBITS_SIZE];
};

void ReadMessages(FileName fname) {
    cerr << "Loading messages..." << endl;
    fname.SetFile("bin/clientmessages-en.bin");

    scoped_ptr<istream> input(fname.Open());
    pstr = PString(*input);
}

string q(const string& in) {
    string out = replace_all_copy(in, "\\", "\\\\");
    replace_all(out, "\"", "\\q");
    replace_all(out, "'", "\\s");
    replace_all(out, "\n", "\\n");
    return out;
}

void writect(string name, int sub) {
    cout << name << "[" << sub << "].";
}

void writeone(string name, string val) {
    cout << name << " \"" << q(val) << "\"" << endl;
}

void writeone(string name, int val) {
    cout << name << " " << val << endl;
}

void writeone(string name, float val) {
    cout << name << " " << val << endl;
}

template <class T>
void readbinsub(FileName fnbase, string fname, Bin<T>& dest,
	void(*cb)(T const&) = 0) {
    cerr << "Loading " << fname << "..." << endl;
    fnbase.SetFile("bin/" + fname);

    scoped_ptr<istream> input(fnbase.Open());
    dest = Bin<T>(*input);

    if (cb) {
	cerr << "Indexing " << fname << "..." << endl;
	foreach(T const& ent, dest) {
	    cb(ent);
	}
    }
}

void readpwrsub(FileName fnbase, string fname, Bin<record::Power>& dest,
	void(*cb)(record::Power const&) = 0) {
    cerr << "Loading " << fname << "..." << endl;
    fnbase.SetFile("bin/" + fname);

    scoped_ptr<istream> input(fnbase.Open());
    dest = Bin<record::Power>(*input, BinRecordExact(record::Power));

    if (cb) {
	cerr << "Indexing " << fname << "..." << endl;
	foreach(record::Power const& ent, dest) {
	    cb(ent);
	}
    }
}


void pcatidxsub(record::PowerCat const& r) {
    string tmp = r.Name;
    to_lower(tmp);
    powercatidx[tmp] = &r;
}

void psetidxsub(record::PowerSet const& r) {
    string tmp = r.FullName;
    to_lower(tmp);
    powersetidx[tmp] = &r;
}

void poweridxsub(record::Power const& r) {
    string tmp = r.FullName;
    to_lower(tmp);
    poweridx[tmp] = &r;
}

void classidxsub(record::CClass const& r) {
    classidx[pstr[r.DisplayName]] = &r;
}

void badgeidxsub(record::Badge const& r) {
    badgeidx[r.Name] = &r;
}

void salvageidxsub(record::Salvage const& r) {
    salvageidx[r.Name] = &r;
}

// FFS, what a hack
static int btmap[] = { 0, 1, 4, 2, 3, 6, 7, 8 };
void costumeidxsub(record::Costume const& c) {
    static int bidx = 0;

    foreach(record::costume::Body const& b, c.Body) {
	foreach(record::costume::Region const& r, b.Region) {
	    foreach(record::costume::BoneSet const& bs, r.BoneSet) {
		foreach(record::costume::GeoSet const& gs, bs.GeoSet) {
		    foreach(record::costume::Info const& i, gs.Info) {
			if (i.DevOnly)
			    continue;

			CostumeKey ckey(btmap[bidx], to_lower_copy(i.Geo),
				to_lower_copy(i.Tex1),
				to_lower_copy(i.Tex2),
				to_lower_copy(i.Fx));

			costumeidx[ckey].push_back(CostumeLoc(&b, &r,
				    &bs, &gs));
		    }
		}
	    }
	}
    }
    ++bidx;
}

void ReadBinFiles(FileName fname) {
    scoped_ptr<istream> input;

    readbinsub(fname, "attrib_names.bin", anames);
    readbinsub(fname, "powercats.bin", powercats, pcatidxsub);
    readbinsub(fname, "powersets.bin", powersets, psetidxsub);
    readpwrsub(fname, "powers.bin", powers, poweridxsub);
    readbinsub(fname, "classes.bin", classes, classidxsub);
    readbinsub(fname, "origins.bin", origins);
    readbinsub(fname, "schedules.bin", schedules);
    readbinsub(fname, "experience.bin", experience);
    readbinsub(fname, "badges.bin", badges, badgeidxsub);
    readbinsub(fname, "Salvage.bin", salvage, salvageidxsub);
    readbinsub(fname, "costume.bin", costumes, costumeidxsub);
    readbinsub(fname, "BodyParts.bin", bodyparts);
}

bool HasBadge(ParseInfo *pi, string badge) {
    const record::Badge *b = badgeidx[badge];

    if (!b) {
	cerr << "Unknown badge: " << badge << endl;
	return false;
    }
    int idx = b->Index;

    return (pi->badgebits[idx >> 3] & (1 << (idx & 7))) != 0;
}

void GrantBadge(ParseInfo *pi, string badge) {
    const record::Badge *b = badgeidx[badge];

    if (!b) {
	cerr << "Unknown badge: " << badge << endl;
	return;
    }
    int idx = b->Index;

    pi->badgebits[idx >> 3] |= 1 << (idx & 7);
}

const char* hexdigits = "0123456789abcdef";

void FinalizeBadges(ParseInfo *pi) {
    int i;
    string o;

    for (i = 0; i < BADGEBITS_SIZE; ++i) {
	o += hexdigits[pi->badgebits[i] >> 4];
	o += hexdigits[pi->badgebits[i] & 0xf];
    }
    writect("Badges", 0);
    writeone("Owned", o);
}

static CostumeLocList GetCostumeLoc(int bt, const string& g, const string& t1,
	const string& t2, const string& fx) {
    CostumeKey ckey(bt, to_lower_copy(g), to_lower_copy(t1),
	    to_lower_copy(t2), to_lower_copy(fx));
    CostumeIdxMap::iterator it;
    
    // exact match has priority
    it = costumeidx.find(ckey);
    if (it != costumeidx.end() && !it->second.empty())
	return it->second;

    // look for a masked version
    ckey.tex2 = "mask";
    it = costumeidx.find(ckey);
    if (it != costumeidx.end())
	return it->second;

    // no matches :(
    return CostumeLocList();
}


#if 0
struct RegionPart {
    RegionPart(record::costume::Region const* r,
	    record::BodyPart const* p): region(r), part(p) {}

    record::costume::Region const* region;
    record::BodyPart const* part;

    private:
    RegionPart();
}

bool operator<(RegionPart const& lhs, RegionPart const& rhs) {
    return ((lhs.region == rhs.region) ?
	    (lhs.part < rhs.part) :
	    (lhs.region < rhs.region));
}
#endif

typedef map<record::costume::GeoSet const*, vector<CostumePart*> > GeoSetPartsMap;
typedef map<record::costume::BoneSet const*, GeoSetPartsMap> BoneSetGeoSetMap;
typedef map<record::costume::Region const*, BoneSetGeoSetMap> RegionBoneSetMap;
void FixupCostumes(ParseInfo *pi) {
    // build a list of all possible matches for each region
    foreach(Costume& c, pi->costumes) {
	RegionBoneSetMap regionmap;

	foreach(CostumePart& part, c.parts) {
	    bool matched = false;

	    foreach(CostumeLoc const& cloc,
		    GetCostumeLoc(c.bodytype, part.geo, part.tex1, part.tex2,
			part.fx)) {
		// add each part to the map, provided it's for the right region
		if (iequals(cloc.geoset->BodyPart, bodyparts[part.slot].Name)) {
		    regionmap[cloc.region][cloc.boneset][cloc.geoset].push_back(&part);
		    matched = true;
/*	DEBUG	    printf("Adding %s, %s, %s, %s to region %s, boneset %s, geoset %s (part %s)\n",
			    part.geo.c_str(), part.tex1.c_str(), part.tex2.c_str(), part.fx.c_str(),
			    cloc.region->Name.c_str(),
			    cloc.boneset->Name.c_str(),
			    pstr[cloc.geoset->DisplayName].c_str(),
			    bodyparts[part.slot].Name.c_str()); */
		}
	    }

	    if (!matched && !(part.empty || part.hasfx)) {
		cerr << "WARNING: Didn't find match for costume part " <<
		    part.geo << ", " << part.tex1 << ", " << part.tex2;
		if (part.hasfx)
		    cerr << " (" << part.fx << ")";
		cerr << endl;
	    }
	}

	// scan each region, sanity check it, and find best match
	// this could probably be done in a single pass, but it reduces
	// complexity to split it up

	for (RegionBoneSetMap::iterator rgi = regionmap.begin();
		rgi != regionmap.end(); ++rgi) {
	    set<record::costume::BoneSet const*> removeBoneSets;

	    // Pass 1, mark bonesets that are missing required body parts
	    for (BoneSetGeoSetMap::iterator bsi = rgi->second.begin();
		    bsi != rgi->second.end(); ++bsi) {
		// get list of required body parts for this boneset
		// from costumes.bin
		set<string> requiredParts;
		foreach(record::costume::GeoSet const& gsc, bsi->first->GeoSet) {
		    if (!gsc.BodyPart.empty())
			requiredParts.insert(to_lower_copy(gsc.BodyPart));
		}

		// look at our list of matches for this boneset
		set<string> foundParts;
		for (GeoSetPartsMap::iterator gsi = bsi->second.begin();
			gsi != bsi->second.end(); ++gsi) {
		    if (!gsi->first->BodyPart.empty())
			foundParts.insert(to_lower_copy(gsi->first->BodyPart));
		}

		set<string> diffParts;
		set_difference(requiredParts.begin(), requiredParts.end(),
			foundParts.begin(), foundParts.end(),
			inserter(diffParts, diffParts.begin()));
		if (diffParts.size() > 0) {
		    removeBoneSets.insert(bsi->first);
/*	DEBUG	    printf("Removing BoneSet %s from region %s because it is missing the following body parts: ", bsi->first->Name.c_str(), rgi->first->Name.c_str());
		    foreach(string bpn, diffParts) {
			printf("%s ", bpn.c_str());
		    }
		    printf("\n"); */
		}
	    }

	    // delete all the ones that we marked
	    foreach(record::costume::BoneSet const* bs, removeBoneSets) {
		rgi->second.erase(bs);
	    }

	    unsigned int bestmatch = 0;
	    BoneSetGeoSetMap::iterator match = rgi->second.end();
	    // now see which boneset has the most matches in it
	    for (BoneSetGeoSetMap::iterator bsi = rgi->second.begin();
		    bsi != rgi->second.end(); ++bsi) {
		if (bsi->second.size() > bestmatch) {
		    match = bsi;
		    bestmatch = bsi->second.size();
		}
	    }

	    // update all the pieces on our chosen set
	    if (bestmatch > 0 && match != rgi->second.end()) {
		for (GeoSetPartsMap::iterator gsi = match->second.begin();
			gsi != match->second.end(); ++gsi) {
		    foreach(CostumePart *cp, gsi->second) {
			cp->region = rgi->first->Name;
			cp->bodyset = match->first->Name;
		    }
		}
	    }
	}
    }
}

#define CA writect("Appearance", cnum);
#define CP writect("CostumeParts", cpnum);

const char *scalenames[] = { "BodyScale", "BoneScale", "HeadScale", "ShoulderScale", "ChestScale", "WaistScale", "HipScale", "LegScale" };
const char *compscalenames[] = { "HeadScales", "BrowScales", "CheekScales", "ChinScales", "CraniumScales", "JawScales", "NoseScales" };

inline unsigned int mkscalepart(float p) {
    int scratch = p * 100;
    if (scratch < 0) scratch = 128 - scratch;
    return static_cast<unsigned int>(scratch);
}

void FinalizeCostumes(ParseInfo *pi) {
    int cnum = 0;
    int cpnum = 0;

    foreach(Costume const& c, pi->costumes) {
	stringstream scales;

	if (cnum == 0) {
	    writeone("ColorSkin", c.skincolor);
	    writeone("BodyType", c.bodytype);
	}

	scales << c.scales;
	float x, y, z;

	CA writeone("BodyType", c.bodytype);
	CA writeone("ColorSkin", c.skincolor);
	CA writeone("ConvertedScale", 1);

	foreach(const char *n, scalenames) {
	    scales >> x;
	    if (x != 0) {
		CA writeone(n, x);
	    }

	    if (cnum == 0 && (n == scalenames[0] || n == scalenames[1]))
		writeone(n, x);
	}

	scales >> x;	    // Consume extra

	foreach(const char *n, compscalenames) {
	    unsigned int tmp = 0;
	    scales >> x >> y >> z;
	    tmp = mkscalepart(x);
	    tmp |= mkscalepart(y) << 8;
	    tmp |= mkscalepart(z) << 16;
	    if (tmp != 0) {
		CA writeone(n, static_cast<int>(tmp));
	    }
	}

	foreach(CostumePart p, c.parts) {
	    if (!p.empty || !p.region.empty() || !p.bodyset.empty()) {
		CP writeone("Geom", p.geo);
		CP writeone("Tex1", p.tex1);
		CP writeone("Tex2", p.tex2);
	    }

	    if (!p.empty) {
		CP writeone("Color1", static_cast<int>(p.color1));
		if (cnum == pi->curcostume)
		    pi->colorfreq[static_cast<int>(p.color1)]++;
		CP writeone("Color2", static_cast<int>(p.color2));
		if (cnum == pi->curcostume)
		    pi->colorfreq[static_cast<int>(p.color2)]++;
	    }

	    if (p.hasfx) {
		CP writeone("FxName", p.fx);
		CP writeone("Color3", static_cast<int>(p.color3));
		CP writeone("Color4", static_cast<int>(p.color4));
	    }

	    if (!p.region.empty()) {
		CP writeone("Region", p.region);
	    }
	    if (!p.bodyset.empty()) {
		CP writeone("BodySet", p.bodyset);
	    }

	    if ((!p.empty || p.hasfx || !p.region.empty() || !p.bodyset.empty()) && cnum > 0) {
		CP writeone("CostumeNum", cnum);
	    }

	    cpnum++;
	}

	cpnum += 10 - (cpnum % 10);

	cnum++;
    }

    writeone("NumCostumeSlots", cnum - 1);
    //container("Ents2", 0);
    //writeone("NumCostumeStored", cnum);
}

struct BonusTable {
    string badge;
    int num;
};

static BonusTable invBonusRecipe[] = {
    { "InventionDeBuff5", 1 },
    { "InventionHeal5", 1 },
    { "InventionMitigation5", 1 },
    { "InventionMez5", 1 },
    { "InventionEndurance5", 1 },
    { "InventionRateOfFire5", 1 },
    { "InventionAccuracy5", 1 },
    { "InventionDamage5", 1 },
    { "InventionTravel5", 1 },
    { "Veteran66b", 5 },
    { "", 0 }
};

static BonusTable invBonusSalvage[] = {
    { "InventionDeBuff3", 2, },
    { "InventionHeal3", 2, },
    { "InventionMitigation3", 2, },
    { "InventionMez3", 2, },
    { "InventionEndurance3", 2, },
    { "InventionRateOfFire3", 2, },
    { "InventionAccuracy3", 2, },
    { "InventionDamage3", 2, },
    { "InventionTravel3", 2, },
    { "AuctionSalvage", 2, },
    { "Veteran66a", 5, },
    { "", 0 }
};

static BonusTable invBonusAuction[] = {
    { "AuctionSeller4", 1, },
    { "AuctionSeller6", 1, },
    { "AuctionSeller8", 1, },
    { "AuctionSeller10", 1, },
    { "AuctionSeller12", 1, },
    { "Veteran66c", 1, },
    { "", 0 }
};

static void FixupInventoryOne(ParseInfo *pi, BonusTable const* bt, int* inv) {
    while (bt->num > 0) {
	if (HasBadge(pi, bt->badge))
	    *inv += bt->num;
	++bt;
    }
}

void FixupInventory(ParseInfo *pi) {
    FixupInventoryOne(pi, invBonusRecipe, &pi->bonusrecipe);
    FixupInventoryOne(pi, invBonusSalvage, &pi->bonussalvage);
    FixupInventoryOne(pi, invBonusAuction, &pi->bonusauction);
}

static struct BadgePowers {
    string badge;
    string power;
} badgepowers[] = {
    { "AtlasSet", "Temporary_Powers.Accolades.The_Atlas_Medallion" }, // 1
    { "FreedomPhalanxSet", "Temporary_Powers.Accolades.Freedom_Phalanx_Reserve" }, // 2
    { "TaskForceCommander", "Temporary_Powers.Accolades.Task_Force_Commander" }, // 3
    { "RiktiWarSet", "Temporary_Powers.Accolades.Vanguard_Medal" }, // 4
    { "DimensionalHopperSet", "Temporary_Powers.Accolades.Portal_Jockey" }, // 5
    { "MagusSet", "Temporary_Powers.Accolades.Eye_Of_The_Magus" }, // 6
    { "CreySet", "Temporary_Powers.Accolades.Crey_CBX-9_Pistol" },    // 8
    { "GeasoftheKindOnes", "Temporary_Powers.Accolades.Geas_Of_The_Kind_Ones" },
    { "Marshal", "Temporary_Powers.Accolades.Marshall" }, // V1
    { "HeadlineStealer", "Temporary_Powers.Accolades.Stolen_Immobilizer_Ray" }, // V2
    { "BornInBattle", "Temporary_Powers.Accolades.Born_In_Battle" }, // V3
    { "HighPainThreshold", "Temporary_Powers.Accolades.High_Pain_Threshold" }, // V4
    { "Demonic", "Temporary_Powers.Accolades.Demonic_Aura" }, // V5
    { "Megalomaniac", "Temporary_Powers.Accolades.Megalomaniac" }, // V6
    { "MayhemInvader", "Temporary_Powers.Accolades.Invader" }, // V7
    { "MayhemForceOfNature", "Temporary_Powers.Accolades.Force_Of_Nature" }, // V8
    { "RIWEAccolade", "Temporary_Powers.Accolades.RIWE_Accolade_Power" },
    { "InventorAccolade", "Temporary_Powers.Accolades.Portable_Workbench" },
    { "", "" }
};

void FixupAccolades(ParseInfo *pi) {
    BadgePowers const* bp = badgepowers;
    while (bp->badge.size() > 0) {
	if (HasBadge(pi, bp->badge)) {
	    pi->powers.insert(pi->powers.begin(), Power(bp->power, 0, 0, false));
	}
	++bp;
    }
}

struct PowerRef {
    PowerRef() {}
    PowerRef(string c, string s, string n): category(c),
	powerset(s), name(n) {}
    explicit PowerRef(string const& src) {
	size_t r1 = src.find('.');
	if (r1 == string::npos)
	    r1 = src.size();
	size_t r2 = src.find('.', r1 + 1);
	if (r2 == string::npos)
	    r2 = src.size();

	category = src.substr(0, r1);
	if (r1 == src.size())
	    powerset = "";
	else
	    powerset = src.substr(r1 + 1, r2 - r1 - 1);

	if (r2 == src.size())
	    name = "";
	else
	    name = src.substr(r2 + 1, src.size() - r2 - 1);
    }

    string category;
    string powerset;
    string name;
};

#define CPW writect("Powers", pnum);
#define CTR writect("Tray", tnum + (pi->curbuild * 90));
#define CTRT writect("Tray", tnum2 + (pi->curbuild * 90));


void WritePower(ParseInfo* pi, Power const& p, PowerRef const& ref, int pnum) {
    string lname = p.id;
    to_lower(lname);

    CPW writeone("PowerId", pnum + 1);
    CPW writeone("CategoryName", ref.category);
    CPW writeone("PowerSetName", ref.powerset);
    CPW writeone("PowerName", ref.name);
    CPW writeone("CreationTime", static_cast<int>(time(NULL) - 946706400));
    if (p.levelbought > 1) {
	CPW writeone("PowerLevelBought", p.levelbought - 1);
    }
    if (pi->setlevels.find(ref.powerset) != pi->setlevels.end()) {
	if (pi->setlevels[ref.powerset] > 1) {
	    CPW writeone("PowerSetLevelBought", pi->setlevels[ref.powerset] - 1);
	}
    }
    if (p.buildnum > 0) {
	CPW writeone("BuildNum", p.buildnum);
    }
    if (p.disabled) {
	CPW writeone("Disabled", 1);
    }

    if (ref.category == "temporary_powers") {
	record::Power const* tpow = poweridx[lname];
	if (tpow && tpow->NumCharges > 0) {
	    CPW writeone("NumCharges", tpow->NumCharges);
	}
	if (tpow && tpow->UsageTime > 0) {
	    CPW writeone("UsageTime", tpow->UsageTime);
	}
	if (tpow && tpow->Lifetime > 0) {
	    CPW writeone("AvailableTime", tpow->Lifetime);
	} else if (tpow && tpow->LifetimeInGame > 0) {
	    CPW writeone("AvailableTime", tpow->LifetimeInGame);
	}
    }

    //CPW writeone("UniqueID", static_cast<int>(((rand() & 0x7fff) << 16) | (rand() & 0xffff)));
}

#define CB writect("Boosts", bnum);

void WriteBoost(Boost const& b, int bnum, int idx) {
    if (idx > 0) {
	CB writeone("Idx", idx);
    }

    PowerRef bref(b.power);
    CB writeone("CategoryName", bref.category);
    CB writeone("PowerSetName", bref.powerset);
    CB writeone("BoostName", bref.name);
    CB writeone("Level", b.level - 1);
    if (b.boost > 0) {
	CB writeone("NumCombines", b.boost);
    }
}

void WriteSlots(Power const& p, int pnum, int* bnum) {
    int idx = 0;

    foreach(Boost const& b, p.slots) {
	if (!b.empty) {
	    writect("Boosts", *bnum);
	    writeone("PowerID", pnum + 1);
	    WriteBoost(b, *bnum, idx);
	    (*bnum)++;
	}

	idx++;
    }

    if (idx > 1) {
	CPW writeone("PowerNumBoostsBought", idx - 1);
    }
}

void FinalizePowers(ParseInfo *pi) {
    deque<PowerRef> primary;
    deque<PowerRef> secondary;
    deque<PowerRef> pool;
    deque<PowerRef> epic;
    deque<PowerRef> inherent;
    deque<PowerRef> temps;

    srand(time(NULL));

    int pnum = 0;
    int bnum = 0;
    int buildlevel[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int buildboosts[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    // Build list of valid primary / secondary sets

    string primarycat = pi->cclass->PrimaryCategory;;
    to_lower(primarycat);
    string secondarycat = pi->cclass->SecondaryCategory;
    to_lower(secondarycat);

    // Scan powers to get some info

    foreach(Power const& p, pi->powers) {
	string id = p.id;
	to_lower(id);
	PowerRef ref(id);

	if (pi->setlevels.find(ref.powerset) == pi->setlevels.end() ||
		p.levelbought < pi->setlevels[ref.powerset]) {
	    pi->setlevels[ref.powerset] = p.levelbought;
	}

	if (p.levelbought > buildlevel[p.buildnum])
	    buildlevel[p.buildnum] = p.levelbought;

	if (p.slots.size() > 1)
	    buildboosts[p.buildnum] += p.slots.size() - 1;

	if (pi->primaryset.empty() && ref.category == primarycat) {
	    PowerRef tmpref(p.id);
	    pi->primaryset = tmpref.powerset;
	}
	if (pi->secondaryset.empty() && ref.category == secondarycat) {
	    PowerRef tmpref(p.id);
	    pi->secondaryset = tmpref.powerset;
	}
    }

    for (int i = 0; i < 8; ++i) {
	int levelbyboost = 0;
	if (buildboosts[i] > 0 && buildboosts[i] <= static_cast<int>(schedules[0].AssignableBoost.size()))
	    levelbyboost = schedules[0].AssignableBoost[buildboosts[i] - 1];

	if (levelbyboost + 1 > buildlevel[i])
	    buildlevel[i] = levelbyboost + 1;

	if (buildlevel[i] > 1) {
	    writect("Ents2", 0);
	    writeone(string("LevelBuild") + lexical_cast<string>(i), buildlevel[i] - 1);
	}
	if (buildlevel[i] > pi->level)
	    pi->level = buildlevel[i];
    }

    // Write out powers

    foreach(Power const& p, pi->powers) {
	string id = p.id;
	to_lower(id);
	PowerRef ref(id);
	if (p.buildnum == pi->curbuild &&
	    (ref.name == "ourportalhero" || ref.name == "ourportalvillain"))
	    temps.push_front(ref);

	if (!(ref.category == "inherent" && ref.powerset == "inherent" &&
		    starts_with(ref.name, "prestige_"))){
	    WritePower(pi, p, ref, pnum);
	    WriteSlots(p, pnum, &bnum);

	    if (p.buildnum == pi->curbuild) {
		if (iequals(ref.category, primarycat))
		    primary.push_back(ref);
		else if (iequals(ref.category, secondarycat))
		    secondary.push_back(ref);
		else if (iequals(ref.category, "Pool"))
		    pool.push_back(ref);
		else if (iequals(ref.category, "Epic"))
		    epic.push_back(ref);
		else if (iequals(ref.category, "Inherent") &&
			iequals(ref.powerset, "Inherent") &&
			(iequals(ref.name, "Sprint") || iequals(ref.name, "Rest")))
		    inherent.push_back(ref);
	    }

	    pnum++;
	}
    }

    // Write standalone enhancements

    int idx = 0;
    foreach(Boost const& b, pi->enhancements) {
	if (!b.empty) {
	    writect("Boosts", bnum);
	    writeone("PowerID", -1);
	    WriteBoost(b, bnum, idx);
	    bnum++;
	}
	idx++;
    }

    // Create trays

    int tnum = 0;
    while (tnum < 10 && primary.size() > 0) {
	PowerRef r = primary.front();
	primary.pop_front();
	CTR writeone("Type", 1);
	CTR writeone("PowerName", r.name);
	CTR writeone("PowerSetName", r.powerset);
	CTR writeone("CategoryName", r.category);
	tnum++;
    }
    tnum = 10;
    while (tnum < 20 && secondary.size() > 0) {
	PowerRef r = secondary.front();
	secondary.pop_front();
	CTR writeone("Type", 1);
	CTR writeone("PowerName", r.name);
	CTR writeone("PowerSetName", r.powerset);
	CTR writeone("CategoryName", r.category);
	tnum++;
    }
    tnum = 20;
    while (tnum < 30 && pool.size() > 0) {
	PowerRef r = pool.front();
	pool.pop_front();
	CTR writeone("Type", 1);
	CTR writeone("PowerName", r.name);
	CTR writeone("PowerSetName", r.powerset);
	CTR writeone("CategoryName", r.category);
	tnum++;
    }
    while (tnum < 30 && epic.size() > 0) {
	PowerRef r = epic.front();
	epic.pop_front();
	CTR writeone("Type", 1);
	CTR writeone("PowerName", r.name);
	CTR writeone("PowerSetName", r.powerset);
	CTR writeone("CategoryName", r.category);
	tnum++;
    }
    int tnum2 = 29;
    while (tnum2 >= tnum && temps.size() > 0) {
	PowerRef r = temps.front();
	temps.pop_front();
	CTRT writeone("Type", 1);
	CTRT writeone("PowerName", r.name);
	CTRT writeone("PowerSetName", r.powerset);
	CTRT writeone("CategoryName", r.category);
	tnum2--;
    }
    while (tnum2 >= tnum && inherent.size() > 0) {
	PowerRef r = inherent.front();
	inherent.pop_front();
	CTRT writeone("Type", 1);
	CTRT writeone("PowerName", r.name);
	CTRT writeone("PowerSetName", r.powerset);
	CTRT writeone("CategoryName", r.category);
	tnum2--;
    }
}

#define CI writect("Inspirations", pos);

void FinalizeInspirations(ParseInfo* pi) {
    int nrows = 0, ncols = 0;

    foreach(int x, schedules[0].InspirationRow) {
	if (x <= pi->level - 1)
	    nrows++;
    }
    foreach(int x, schedules[0].InspirationCol) {
	if (x <= pi->level - 1)
	    ncols++;
    }

    int pos = 0;
    foreach(string insp, pi->inspirations) {
	to_lower(insp);
	PowerRef ref(insp);
	if ((pos % ncols) > 0) {
	    CI writeone("Col", pos % ncols);
	}
	if ((pos / ncols) > 0) {
	    CI writeone("Row", pos / ncols);
	}
	CI writeone("CategoryName", ref.category);
	CI writeone("PowerSetName", ref.powerset);
	CI writeone("PowerName", ref.name);

	pos++;
	if (pos > nrows * ncols)    // Oops!
	    break;
    }
}

void FinalizeCharacter(ParseInfo *pi) {
    // Grant Passport badge for all who enter here
    GrantBadge(pi, "ServerTransfer");

    // Static stuff that doesn't need to change
    writeone("Endurance", 100);

    // Find character's combat level
    int clevel = 0;
    foreach(int expr, experience[0].ExperienceRequired) {
	if (pi->xp >= expr)
	    clevel++;
    }

    // Output stuff like hit points
    float hp = pi->cclass->AttribMaxTable[0].HitPoints[clevel-1];
    writeone("HitPoints", hp);
    writect("Ents2", 0);
    writeone("MaxHitPoints", hp);

    FixupCostumes(pi);
    FinalizeCostumes(pi);

/* NOT WORKING
    int nm = 0, ncol = 0x909090ff;
    for (map<int, int>::iterator icf = pi->colorfreq.begin();
	    icf != pi->colorfreq.end(); ++icf) {
	if (icf->second > nm) {
	    ncol = icf->first;
	    nm = icf->second;
	}
    }
    ncol = (ncol << 8) | 0xff;
    for (int i = 1; i <= 124; ++i) {
	container("Windows", i);
	writeone("Color", ncol);
	container("Windows", i);
	writeone("BackColor", 136);
	container("Windows", i);
	writeone("Scale", 1);
    }
*/

    int curbody = pi->costumes[pi->curcostume].bodytype;
    int gender = 1;	// neutral
    if (curbody == 0 || curbody == 2) {
	gender = 2;	// male
    } else if (curbody == 1)
	gender = 3;	// female
    writeone("Gender", gender);
    writeone("NameGender", gender);

    FixupAccolades(pi);
    FinalizePowers(pi);

    writeone("Level", pi->level - 1);
    FinalizeInspirations(pi);

    if (!pi->primaryset.empty()) {
	writect("Ents2", 0);
	writeone("OriginalPrimary", pi->primaryset);
    }
    if (!pi->secondaryset.empty()) {
	writect("Ents2", 0);
	writeone("OriginalSecondary", pi->secondaryset);
    }

    // Figure out character alignment
    int playertype = 0;
    int playersubtype = 0;
    int praetorianprogress = 0;

    if (pi->alignment == "(Neutral)") {
	praetorianprogress = 6;
    } else if (pi->alignment == "Villain") {
	playertype = 1;
    } else if (pi->alignment == "Vigilante") {
	playersubtype = 2;
    } else if (pi->alignment == "Rogue") {
	playertype = 1;
	playersubtype = 2;
    } else if (pi->alignment == "Resistance") {
	praetorianprogress = 2;
    } else if (pi->alignment == "Loyalist") {
	playertype = 1;
	praetorianprogress = 2;
    } else if (pi->dimension == "Praetorian")
	praetorianprogress = 2;

    // Sentinel+ doesn't intepret PraetorianProgress correctly,
    // work around it by checking badges
    if (praetorianprogress == 0 && (HasBadge(pi, "P_ResistanceBadge") ||
	HasBadge(pi, "P_LoyalistBadge"))) {
	praetorianprogress = 3;
    }

    int zone = 0;
    float x = 0, y = 0, z = 0;
    // Set starting zone and coordinates
    if (praetorianprogress == 6) {
	zone = 41;	    // Destroyed Galaxy
	x = 343.9992;
	y = 1.35798;
	z = 277.9865;
    } else if (praetorianprogress > 0 && praetorianprogress < 3) {
	zone = 36;	    // Nova Praetoria
	x = -4669.14656;
	y = 40.499949;
	z = -240.0558;
    } else if (playertype == 1) {
	zone = 71;	    // Mercy Island
	x = -1261.9993;
	y = 238.500;
	z = -179.000;
    } else {
	zone = 1;	    // Atlas Park
	x = 106.4556;
	y = -0.57388;
	z = -114.45343;
    }

    writeone("StaticMapId", zone);
    writeone("PosX", x);
    writeone("PosY", y);
    writeone("PosZ", z);
    writeone("SpawnTarget", "NewPlayer");

    writeone("PlayerType", playertype);
    writect("Ents2", 0);
    writeone("PlayerSubType", playersubtype);
    writect("Ents2", 0);
    writeone("PraetorianProgress", praetorianprogress);

    if (playertype) {
	writect("Ents2", 0);
	writeone("InfluenceType", 1);
    }

    FixupInventory(pi);
    writect("Ents2", 0);
    writeone("RecipeInvBonus", pi->bonusrecipe);
    writect("Ents2", 0);
    writeone("SalvageInvBonus", pi->bonussalvage);
    writect("Ents2", 0);
    writeone("AuctionInvBonus", pi->bonusauction);

    // Static stuff that never changes

    writeone("CurrentAltTray", 1);
    writect("Ents2", 0);
    writeone("CurrentAlt2Tray", 2);
    writect("Ents2", 0);
    writeone("HelperStatus", 3);
    writeone("UiSettings", 1073766435);
    writect("Ents2", 0);
    writeone("UiSettings2", 268484608);
    writect("Ents2", 0);
    writeone("UiSettings3", -2117599232);
    writect("Ents2", 0);
    writeone("UiSettings4", 6);
    writect("Ents2", 0);
    writeone("MapOptions", -258);
    writect("Ents2", 0);
    writeone("MapOptions2", 14);
    writect("Ents2", 0);
    writeone("MapOptionRevision", 4);

    writeone("DateCreated", "2012-10-08 16:00:00.000");
    writeone("LastActive", "2012-10-08 16:00:00.000");

    // Grant misc badges based on character state

    FinalizeBadges(pi);
}

// Power

void ElemEndPower(ParseInfo *pi, const string& el) {
    if (el == "id")
	pi->powers.back().id = pi->charbuf;
    else if (el == "level")
	pi->powers.back().levelbought = lexical_cast<int>(pi->charbuf);
}

// Slot

void ElemStartSlot(ParseInfo *pi, const string& el, map<string, string>& attr) {
    if (el != "slot")
	return;

    if (attr.find("empty") != attr.end()) {
	pi->powers.back().slots.push_back(Boost(true));
	return;
    }

    pi->powers.back().slots.push_back(Boost(lexical_cast<int>(attr["level"]),
		lexical_cast<int>(attr["boost"]), ""));
}

void ElemEndSlot(ParseInfo *pi, const string& el) {
    if (el != "slot")
	return;

    pi->powers.back().slots.back().power = pi->charbuf;
}

// Salvage

void ElemStartSalvage(ParseInfo *pi, const string& el, map<string, string>& attr) {
    if (el == "salvage") {
	pi->regs[0] = lexical_cast<int>(attr["quantity"]);
    }
}

void ElemEndSalvage(ParseInfo *pi, const string& el) {
    if (el != "salvage")
	return;

    record::Salvage const* s = salvageidx[pi->charbuf];

    // If player has these, they've already seen the popup
    if (s && s->Type == 1)
	GrantBadge(pi, "InventionFirstSalvage");
    if (pi->charbuf == "S_MeritReward")
	GrantBadge(pi, "FirstMeritReward");
    if (pi->charbuf == "S_ArchitectTicket")
	GrantBadge(pi, "FirstTicket");

    writect(pi->tagstack.back() == "vault" ? "InvStoredSalvage0" : "InvSalvage0", 0);
    writeone(pi->charbuf, pi->regs[0]);
}

// Recipe

void ElemStartRecipe(ParseInfo *pi, const string& el, map<string, string>& attr) {
    if (el == "recipe") {
	pi->regs[0] = lexical_cast<int>(attr["quantity"]);
    }
}

void ElemEndRecipe(ParseInfo *pi, const string& el) {
    if (el != "recipe")
	return;

    writect("InvRecipeInvention", pi->regs[2]);
    writeone(string("c") + lexical_cast<string>(pi->regs[1]) + "Type",
	    pi->charbuf);
    writect("InvRecipeInvention", pi->regs[2]);
    writeone(string("c") + lexical_cast<string>(pi->regs[1]) + "Amount",
	    pi->regs[0]);

    pi->regs[1]++;
    if (pi->regs[1] == 450) {
	pi->regs[1] = 0;
	pi->regs[2]++;
    }
}

// Incarnate

void ElemStartIncarnate(ParseInfo *pi, const string& el, map<string, string>& attr) {
    if (el != "ability")
	return;

    pi->regs[0] = (attr.find("slotted") == attr.end()) ? 1 : 0;
}

void ElemEndIncarnate(ParseInfo *pi, const string& el) {
    if (el != "ability")
	return;

    pi->powers.push_back(Power(pi->charbuf, 50, 0, pi->regs[0]));
}

// Enhancement

void ElemStartEnhancement(ParseInfo *pi, const string& el, map<string, string>& attr) {
    if (el != "enhancement")
	return;

    pi->regs[0] = lexical_cast<int>(attr["level"]);
    pi->regs[1] = lexical_cast<int>(attr["boost"]);
}

void ElemEndEnhancement(ParseInfo *pi, const string& el) {
    if (el != "enhancement")
	return;

    pi->enhancements.push_back(Boost(pi->regs[0], pi->regs[1], pi->charbuf));
}

// Costume

void ElemEndCostume(ParseInfo *pi, const string& el) {
    if (el == "bodytype")
	pi->costumes.back().bodytype = lexical_cast<int>(pi->charbuf);
    else if (el == "skincolor") {
	unsigned int scolor;
	stringstream ss;
	ss << std::hex << pi->charbuf;
	ss >> scolor;
	pi->costumes.back().skincolor = static_cast<int>(scolor);
    }
    else if (el == "scales")
	pi->costumes.back().scales = pi->charbuf;
}

// Character

void ElemEndCharacter(ParseInfo *pi, const string& el) {
    if (pi->charbuf.size() > 0) {
	if (el == "name") {
	    writeone("Name", pi->charbuf);
	} else if (el == "title") {
	    vector<string> svec;

	    split(svec, pi->charbuf, is_any_of(" "));
	    foreach(string p, svec) {
		bool found = false;

		if (p == "The") {
		    writect("Ents2", 0);
		    writeone("TitleTheText", p);
		    continue;
		}

		foreach(const char *ptest, commontitles) {
		    if (!found && p == ptest) {
			writeone("TitleCommon", p);
			found = true;
		    }
		}

		if (!found)
		    writeone("TitleOrigin", p);
	    }
	} else if (el == "description") {
	    writeone("Description", pi->charbuf);
	} else if (el == "origin") {
	    to_lower(pi->charbuf);
	    writeone("Origin", pi->charbuf);
	} else if (el == "archetype") {
	    pi->cclass = classidx[pi->charbuf];
	    if (!pi->cclass) {
		cerr << "Unknown archetype (!): " << pi->charbuf << ". Expect a crash soon!" << endl;
	    } else {
		string tmp = pi->cclass->Name;
		to_lower(tmp);
		writeone("Class", tmp);
	    }
	} else if (el == "alignment") {
	    pi->alignment = pi->charbuf;
	} else if (el == "dimension") {
	    pi->dimension = pi->charbuf;
	} else if (el == "inf") {
	    writeone("InfluencePoints", lexical_cast<int>(pi->charbuf));
	} else if (el == "xp") {
	    pi->xp = lexical_cast<int>(pi->charbuf);
	    writeone("ExperiencePoints", pi->xp);
	} else if (el == "costume") {
	    pi->curcostume = lexical_cast<int>(pi->charbuf);
	    if (pi->curcostume > 0)
		writeone("CurrentCostume", pi->curcostume);
	} else if (el == "build" && pi->charbuf != "0") {
	    writect("Ents2", 0);
	    pi->curbuild = lexical_cast<int>(pi->charbuf);
	    writeone("CurBuild", pi->curbuild);
	} else if (el == "salvagecap") {
	    writect("Ents2", 0);
	    writeone("SalvageInvTotal", lexical_cast<int>(pi->charbuf));
	} else if (el == "vaultcap") {
	    writect("Ents2", 0);
	    writeone("StoredSalvageInvTotal", lexical_cast<int>(pi->charbuf));
	} else if (el == "recipecap") {
	    writect("Ents2", 0);
	    writeone("RecipeInvTotal", lexical_cast<int>(pi->charbuf));
	}
    }
}

// libexpat XML Callbacks

void ElemStart(void *data, const char *elc, const char **attrc) {
    ParseInfo *pi = (ParseInfo*)data;
    string el = elc;
    map<string, string> attr;

    for (; attrc[0]; attrc += 2) {
	attr[string(attrc[0])] = string(attrc[1]);
    }

    pi->charbuf.clear();

    if (pi->tagstack.size()) {
	if (pi->tagstack.back() == "salvages" || pi->tagstack.back() == "vault")
	    ElemStartSalvage(pi, el, attr);
	else if (pi->tagstack.back() == "recipes")
	    ElemStartRecipe(pi, el, attr);
	else if (pi->tagstack.back() == "enhancements")
	    ElemStartEnhancement(pi, el, attr);
	else if (pi->tagstack.back() == "incarnate")
	    ElemStartIncarnate(pi, el, attr);
	else if (pi->tagstack.back() == "costumes" && el == "costume")
	    pi->costumes.push_back(Costume());
	else if (pi->tagstack.back() == "build" && el == "power") {
	    pi->powers.push_back(Power());
	    pi->powers.back().buildnum = pi->regs[3];
	} else if (pi->tagstack.back() == "slots")
	    ElemStartSlot(pi, el, attr);
    }

    if (el == "recipes") {
	pi->regs[1] = 0;
	pi->regs[2] = 0;
    } else if (el == "builds") {
	pi->regs[3] = 0;
    }

    pi->tagstack.push_back(el);
}

void ElemEnd(void *data, const char *elc) {
    ParseInfo *pi = (ParseInfo*)data;
    string el = elc;

    pi->tagstack.pop_back();

    if (pi->tagstack.size()) {
	if (pi->tagstack.back() == "character")
	    ElemEndCharacter(pi, el);
	else if (pi->tagstack.back() == "salvages" || pi->tagstack.back() == "vault")
	    ElemEndSalvage(pi, el);
	else if (pi->tagstack.back() == "recipes")
	    ElemEndRecipe(pi, el);
	else if (pi->tagstack.back() == "badges" && el == "badge")
	    GrantBadge(pi, pi->charbuf);
	else if (pi->tagstack.back() == "enhancements")
	    ElemEndEnhancement(pi, el);
	else if (pi->tagstack.back() == "inspirations" && el == "inspiration")
	    pi->inspirations.push_back(pi->charbuf);
	else if (pi->tagstack.back() == "incarnate")
	    ElemEndIncarnate(pi, el);
	else if ((pi->tagstack.back() == "temporary" ||
		    pi->tagstack.back() == "inherent") && el == "power")
	    pi->powers.push_back(Power(pi->charbuf, 1, 0, false));
	else if (pi->tagstack.back() == "costume")
	    ElemEndCostume(pi, el);
	else if (pi->tagstack.back() == "costumes" && el == "costume")
	    pi->curpartidx = 0;
	else if (pi->tagstack.back() == "parts")
	    pi->costumes.back().parts.push_back(CostumePart(pi->curpartidx++, pi->charbuf));
	else if (pi->tagstack.back() == "builds" && el == "build")
	    pi->regs[3]++;
	else if (pi->tagstack.back() == "power")
	    ElemEndPower(pi, el);
	else if (pi->tagstack.back() == "slots")
	    ElemEndSlot(pi, el);
    } else if (el == "character")
	FinalizeCharacter(pi);

    pi->charbuf.clear();
}

void CharData(void *data, const char *s, int len) {
    ParseInfo *pi = (ParseInfo*)data;
    string str(s, len);

    pi->charbuf += str;
}

// Actual conversion

void DoConvert(string fname, int aid, string aname, bool override) {
/*    if (!ValidateFile(fname.c_str())) {
	if (override)
	    cerr << "WARNING: ";
	cerr << "XML has been tampered with!" << endl;
	if (!override)
	    return;
    } */

    ParseInfo pinfo;
    ifstream input(fname.c_str());
    char buf[8192];

    XML_Parser p = XML_ParserCreate(NULL);
    XML_SetUserData(p, &pinfo);
    XML_SetElementHandler(p, ElemStart, ElemEnd);
    XML_SetCharacterDataHandler(p, CharData);

    writeone("AuthId", aid);
    writeone("AuthName", aname);

    for (;;) {
	int done;
	input.getline(buf, sizeof(buf));
	done = input.eof();

	if (!XML_Parse(p, buf, strlen(buf), done)) {
	    cerr << "Parse error at line " << XML_GetCurrentLineNumber(p)
		<< ":" << endl << XML_ErrorString(XML_GetErrorCode(p)) << endl;
	    return;
	}

	if (done)
	    break;
    }

    XML_ParserFree(p);
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
	cerr << "Usage: xml2db [authid] [authname] [bin.pigg] [xmlfile]" << endl;
	return 1;
    }

    int authid = atoi(argv[1]);
    string authname = string(argv[2]);
    FileName piggfname;
    piggfname.SetPigg(argv[3]);
    string xmlfname = string(argv[4]);
    
    bool override = false;
    if (argc == 6 && iequals(string(argv[5]), "-o"))
	override = true;

    try {
	ReadMessages(piggfname);
	ReadBinFiles(piggfname);
	DoConvert(xmlfname, authid, authname, override);
    } catch (std::exception& e) {
	cerr << "Error: " << e.what() << endl;
	return 1;
    }

    return 0;
}
