#include "SOPFormula.h"

void ProductTerm::add_term(std::pair<IID<IPid>, int> term) {
    if (has(term)) return;

    terms.push_back(term);
    result = RESULT::DEPENDENT;
}

RESULT ProductTerm::check_evaluation(std::pair<IID<IPid>, int> term) {
    if (!unit()) return result;
    if (terms.front() == term) return RESULT::TRUE;
    if (terms.begin()->first == term.first) return RESULT::FALSE;
        
    return result;
}

RESULT ProductTerm::evaluate(std::pair<IID<IPid>, int> term) {
    if (result == RESULT::FALSE || result == RESULT::TRUE) return result;

    for (auto it = terms.begin(); it != terms.end();) {
        if (it->first != term.first) {it++; continue;}

        if (it->second != it->second) { // same read, different value
            result = RESULT::FALSE;
            return result;
        }
        else if (unit()) { // same read, same value and unit term 
            result = RESULT::TRUE;
            return result;
        }
        else { // same read, same value but has other terms
            it = terms.erase(it);
        }
    }

    assert(result == RESULT::DEPENDENT);
    return RESULT::DEPENDENT;
}


bool ProductTerm::has(std::pair<IID<IPid>, int> term) {
    for (auto it = terms.begin(); it != terms.end(); it++) {
        if ((*it) == term) return true;
    }

    return false;
}

RESULT ProductTerm::reduce(std::pair<IID<IPid>, int> term) {
    for (auto it = terms.begin(); it != terms.end();) {
        if ((*it) == term) { // found term grounded to true
            if (unit()) {
                result = RESULT::TRUE;
            }
            it = terms.erase(it);
            break;
        }
        
        if (it->first == term.first) { // found term's event with another value, grounded to false
            result = RESULT::FALSE;
            break;
        }

        it++;
    }

    return result;
}

string ProductTerm::to_string() {
    int cnt = 0;
    std::string s = "(";

    auto it = terms.begin();
    for (; it != terms.end(); it++, cnt++)
    {
        if (cnt == terms.size() - 1)
            break;
        s = s + "[" + std::to_string(it->first.get_pid()) + ":" + std::to_string(it->first.get_index()) + "]";
        s = s + "(" + std::to_string(it->second) + ") ^ ";
    }

    s = s + "[" + std::to_string(it->first.get_pid()) + ":" + std::to_string(it->first.get_index()) + "]";
    s = s + "(" + std::to_string(it->second) + "))";
    return s;
}

string ProductTerm::result_to_string(RESULT result) {
    switch (result) {
        case RESULT::TRUE: return "TRUE"; break;
        case RESULT::FALSE: return "FALSE"; break;
        case RESULT::DEPENDENT: return "DEPENDENT"; break;
        
        default: break;
    }
    return "";
}

bool ProductTerm::operator==(ProductTerm product_term) {
    if (terms.size() != product_term.terms.size()) 
        return false;

    for (auto it = terms.begin(); it != terms.end(); it++) {
        if (!product_term.has(*it))
            return false;
    }

    return true;
}

void ProductTerm::operator&&(ProductTerm product_term) {
    for (auto it = product_term.terms.begin(); it != product_term.terms.end(); it++) {
        add_term(*it);
    }
}

SOPFormula::SOPFormula(std::pair<IID<IPid>, int> term) {
    ProductTerm product_term(term);
    terms.push_back(product_term);
    result = RESULT::DEPENDENT;
}

RESULT SOPFormula::check_evaluation(std::pair<IID<IPid>, int> term) {
    bool result_false = true;
    for (auto it = terms.begin(); it != terms.end(); it++) {
        RESULT product_term_result = it->check_evaluation(term);
        if (product_term_result == RESULT::TRUE) {
            return RESULT::TRUE;
        }

        if (product_term_result == RESULT::DEPENDENT) {
            result_false = false;
        }
    }

    if (result_false) return RESULT::FALSE;
    return RESULT::DEPENDENT;
}

RESULT SOPFormula::evaluate(std::pair<IID<IPid>, int> term) {
    if (result == RESULT::FALSE || result == RESULT::TRUE) return result;

    bool result_true = false;
    for (auto it = terms.begin(); it != terms.end();) {
        RESULT product_term_result = it->evaluate(term);
        if (product_term_result == RESULT::TRUE) {
            result_true = true;
            break;
        }

        if (product_term_result == RESULT::FALSE) {
            it = terms.erase(it);
        }
        else it++;
    }

    if (result_true) {
        terms.clear();
        result = RESULT::TRUE;
        return result;
    }

    if (terms.empty()) { // all product terms were removed beacuse they were FALSE or INIT
        result = RESULT::FALSE;
        return result;
    }

    assert(result == RESULT::DEPENDENT);
    return result;
}


bool SOPFormula::has(std::pair<IID<IPid>, int> term) {
    for (auto it = terms.begin(); it != terms.end(); it++) {
        if (!it->unit()) continue;
        if (it->has(term)) return true;
    }

    return false;
}

bool SOPFormula::has_product_term(ProductTerm product_term) {
    if (terms.empty()) return false;

    for (auto it = terms.begin(); it != terms.end(); it++) {
        if ((*it) == product_term) 
            return true;
    }

    return false;
}

void SOPFormula::reduce(std::pair<IID<IPid>, int> term) {
    for (auto it = terms.begin(); it != terms.end();) {
        RESULT product_term_result = it->reduce(term);
        if (product_term_result == RESULT::TRUE) {
            result = RESULT::TRUE;
            break;
        }

        if (product_term_result == RESULT::FALSE) {
            it = terms.erase(it);
            continue;
        }

        it++;
    }

    if (result == RESULT::TRUE) {
        terms.clear();
        return;
    }

    if (terms.empty()) { // all terms FALSE
        terms.clear();
        result = RESULT::FALSE;
    }
}

void SOPFormula::remove_terms_of_term(std::pair<IID<IPid>, int> term) {
    for (auto it = terms.begin(); it != terms.end();) {
        if (it->has(term)) {
            it = terms.erase(it);
        }
        else it++;
    }

    if (terms.empty()) {llvm::outs() << "must not be empty\n";}
}

void SOPFormula::operator||(SOPFormula &formula) {
    if (result == RESULT::TRUE || formula.result == RESULT::FALSE) return;
    
    if (formula.result == RESULT::TRUE) {
        terms.clear();
        result = RESULT::TRUE;
        return;
    }

    if (result == RESULT::FALSE) {
        terms = formula.terms;
        result = formula.result;
        return;
    }

    for (auto it = formula.begin(); it != formula.end(); it++) {
        (*this) || (*it);
    }
}

void SOPFormula::operator||(ProductTerm &product_term) {
    if (result == RESULT::TRUE) return;
    if (has_product_term(product_term)) return;

    terms.push_back(product_term);
    result = RESULT::DEPENDENT;
}

void SOPFormula::operator||(std::pair<IID<IPid>, int> term) {
    ProductTerm product_term(term);
    (*this) || product_term;
}

void SOPFormula::operator&&(SOPFormula &formula) {
    llvm::outs() << "in &&, is formula.result=FALSE? " << (formula.result == RESULT::FALSE) << "\n";
    if (formula.result == RESULT::TRUE) return; // formula && TRUE = formula
    if (result == RESULT::FALSE) return; // this already FALSE, will remain FALSE after && with anything 

    if (formula.result == RESULT::FALSE) { // && will FALSE, will make this FALSE
        terms.clear();
        result = RESULT::FALSE;
        return;
    }

    if (result == RESULT::TRUE) { // this is TRUE so && with formula will return formula 
        terms = formula.terms;
        result = formula.result;
        return;
    }

    auto it = formula.begin();
    SOPFormula conjunction = (*this);
    conjunction && (*it);
    it++;
    
    for (; it != formula.end(); it++) {
        SOPFormula sop_term = (*this);
        sop_term && (*it);
        conjunction || sop_term;
    }

    terms.clear();
    terms = conjunction.terms;
    result = conjunction.result;
}

void SOPFormula::operator&&(ProductTerm &product_term) {
    if (result == RESULT::FALSE) return; // will remain FALSE

    if (result == RESULT::TRUE) {
        terms.push_back(product_term);
        result == RESULT::DEPENDENT;
        return;
    }

    for (auto it = terms.begin();  it != terms.end(); it++) {
        (*it) && product_term;
    }
}

void SOPFormula::operator&&(std::pair<IID<IPid>, int> term) {
    if (result == RESULT::FALSE) return; // will remain FALSE
    
    if (result == RESULT::TRUE) {
        terms.push_back(ProductTerm(term));
        result == RESULT::DEPENDENT;
        return;
    }

    for (auto it = terms.begin(); it != terms.end(); it++) {
        it->add_term(term);
    }
}

bool SOPFormula::operator==(SOPFormula &formula) {
    if (size() != formula.size()) return false;

    for (auto it = terms.begin(); it != terms.end(); it++) {
        if (!formula.has_product_term(*it))
            return false;
    }

    return true;
}

string SOPFormula::to_string() {
    if (result == RESULT::FALSE)
        return "FALSE";
    if (result == RESULT::TRUE)
        return "TRUE";

    int cnt = 0;

    std::string s = "(";

    auto it = terms.begin();
    for (; it != terms.end(); it++, cnt++) {
        if (cnt == terms.size()-1)
            break;
        s = s + it->to_string() + " v ";
    }

    s = s + it->to_string() + ")";
    return s;
}