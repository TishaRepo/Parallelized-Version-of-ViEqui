#ifndef __SOP_FORMLA_H__
#define __SOP_FORMULA_H__

#include <vector>
#include <IID.h>

using namespace std;

typedef int IPid;

enum class RESULT {
    TRUE,
    FALSE,
    DEPENDENT,
    INIT
};

class ProductTerm {
    // respresenting conjuncted (read event,value) pairs
    vector<pair<IID<IPid>, int>> terms;
    RESULT result;

public:
    ProductTerm() {result = RESULT::INIT;}
    ProductTerm(std::pair<IID<IPid>, int> term) {terms.push_back(term); result = RESULT::DEPENDENT;}

    ~ProductTerm() {terms.clear();}

    // is unit term?
    bool unit() { return (terms.size() == 1); }
    // insert term in conjuction with existing terms
    void add_term(std::pair<IID<IPid>, int> term);
    // check what formula evaluates to but don't evaluate (modify the formula)
    RESULT check_evaluation(std::pair<IID<IPid>, int> term);
    /* evalute and update the formula  as follows 
       if term (read,value) = term then 
       ground (read, value) with true
       ground (read, value') with false
    */
    RESULT evaluate(std::pair<IID<IPid>, int> term);
    // check if product formula has a (event,value) = term
    bool has(std::pair<IID<IPid>, int> term);
    // remove term from terms
    RESULT reduce(std::pair<IID<IPid>, int> term);

    vector<pair<IID<IPid>, int>>::iterator begin() { return terms.begin(); }
    vector<pair<IID<IPid>, int>>::iterator end() { return terms.end(); }

    string to_string();
    string result_to_string(RESULT result);

    bool operator==(ProductTerm term);
};

class SOPFormula {
    vector<ProductTerm> terms;
    RESULT result;

public:
    SOPFormula() {result = RESULT::INIT;}
    SOPFormula(std::pair<IID<IPid>, int> term);

    ~SOPFormula() {terms.clear();}

    // empty formula (has no terms)
    bool empty() {return terms.empty();}
    // size of terms
    int size() {return terms.size();}
    // check what formula evaluates to but don't evaluate (modify the formula)
    RESULT check_evaluation(std::pair<IID<IPid>, int> term);
    // evalute and update the formula  
    RESULT evaluate(std::pair<IID<IPid>, int> term);
    // check if formula has a unit productTerm (event,value) = term
    bool has(std::pair<IID<IPid>, int> term);
    // check if formula has a productTerm = product_term
    bool has_product_term(ProductTerm product_term);
    // remove term from productTerms that contain it
    void reduce(std::pair<IID<IPid>, int> term);
    // remove ProductTerms that contain term
    void remove_terms_of_term(std::pair<IID<IPid>, int> term);

    vector<ProductTerm>::iterator begin() { return terms.begin(); }
    vector<ProductTerm>::iterator end() { return terms.end(); }

    void operator||(SOPFormula &formula);
    void operator||(ProductTerm &product_term);
    void operator||(std::pair<IID<IPid>, int> term);
    void operator&&(SOPFormula &formula);
    void operator&&(ProductTerm &product_term);
    void operator&&(std::pair<IID<IPid>, int> term);
    void operator=(SOPFormula f) { terms = f.terms; result = f.result; }
    bool operator==(SOPFormula &f);

    void clear() { terms.clear(); result = RESULT::INIT; }

    string to_string();
};

#endif