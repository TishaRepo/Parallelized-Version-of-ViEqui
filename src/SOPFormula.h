#ifndef __SOP_FORMLA_H__
#define __SOP_FORMULA_H__

#include <forward_list>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <assert.h>
#include <algorithm>

// [rmnt]: Represents the 3 possible states of a formula (or a term). It is either true under all settings of its variables, false under all settings of its variables, or has a value dependent on the setting.
enum class RESULT
{
    TRUE,
    FALSE,
    DEPENDENT,
    INIT
};

template <class T>
class ProductTerm
{
private:
    bool initPhase = true;

public:
    std::unordered_map<T, uint8_t> objToVal;
    std::unordered_set<T> objects;
    RESULT result = RESULT::INIT;

    ProductTerm() {}
    ProductTerm(std::pair<T, int> p) { addConstraint(p.first, p.second); }

    void addConstraint(T var, int val)
    {
        // [rmnt]: Ensure we don't add a variable whose value has already been instantiated
        assert(sanity());
        initPhase = false;
        result = RESULT::DEPENDENT;
        assert(objects.find(var) == objects.end());
        assert(objToVal.find(var) == objToVal.end());

        objects.insert(var);
        objToVal.insert({var, val});
        assert(sanity());
    }

    RESULT check_evaluation(std::unordered_map<T, int> valueEnv)
    {
        // [rmnt]: Evaluating an unitialized term is meaningless
        assert(sanity());
        assert(result != RESULT::INIT);
        assert(!initPhase);

        RESULT ret_result = result;

        if (objects.empty())
        {
            assert(ret_result == RESULT::TRUE || ret_result == RESULT::FALSE);
            assert(sanity());
            return ret_result;
        }

        bool isDependent = false;
        std::unordered_set<T> trueVars;
        for (auto i = objects.begin(); i != objects.end(); i++)
        {
            auto got = valueEnv.find(*i);
            if (got != valueEnv.end())
            {
                if (objToVal.at(*i) != got->second)
                {
                    ret_result = RESULT::FALSE;
                    assert(sanity());
                    return ret_result;
                }
                else
                {
                    trueVars.insert(*i);
                }
            }
            else
            {
                ret_result = RESULT::DEPENDENT;
                isDependent = true;
            }
        }

        if (!isDependent)
        {
            ret_result = RESULT::TRUE;
            assert(sanity());
            return ret_result;
        }

        assert(ret_result == RESULT::DEPENDENT);
        // [rmnt]: Clean up true vars

        assert(sanity());
        return ret_result;
    }

    RESULT evaluate(std::unordered_map<T, int> valueEnv)
    {
        // [rmnt]: Evaluating an unitialized term is meaningless
        assert(sanity());
        assert(result != RESULT::INIT);
        assert(!initPhase);
        if (objects.empty())
        {
            assert(result == RESULT::TRUE || result == RESULT::FALSE);
            assert(sanity());
            return result;
        }

        bool isDependent = false;
        std::unordered_set<T> trueVars;
        for (auto i = objects.begin(); i != objects.end(); i++)
        {
            auto got = valueEnv.find(*i);
            if (got != valueEnv.end())
            {
                if (objToVal.at(*i) != got->second)
                {
                    objects.clear();
                    objToVal.clear();
                    result = RESULT::FALSE;
                    assert(sanity());
                    return result;
                }
                else
                {
                    trueVars.insert(*i);
                }
            }
            else
            {
                result = RESULT::DEPENDENT;
                isDependent = true;
            }
        }

        if (!isDependent)
        {
            objects.clear();
            objToVal.clear();
            result = RESULT::TRUE;
            assert(sanity());
            return result;
        }

        assert(result == RESULT::DEPENDENT);
        // [rmnt]: Clean up true vars
        for (auto j = trueVars.begin(); j != trueVars.end(); j++)
        {
            objToVal.erase(*j);
            objects.erase(*j);
        }
        // objToVal.erase(std::remove_if(objToVal.begin(), objToVal.end(), [trueVars]({T var, uint8_t val}){
        //                    return (trueVars.find(var) != trueVars.end());
        //                }),
        //                objToVal.end());
        // objects.erase(std::remove_if(objects.begin(), objects.end(), [trueVars]({T var, uint8_t val}){
        //                   return (trueVars.find(var) != trueVars.end());
        //               }),
        //               objects.end());

        assert(sanity());
        return result;
    }

    bool is_term_of_object(T object)
    {
        if (!unit())
            return false;
        if (objToVal.begin()->first != object)
            return false;
        return true;
    }

    bool unit()
    {
        return (objToVal.size() == 1);
    }

    bool in(std::pair<T, int> term)
    {
        for (auto it = objToVal.begin(); it != objToVal.end(); it++)
        {
            if (it->first == term.first && it->second == term.second)
                return true;
        }

        return false;
    }

    std::string to_string()
    {
        int size = objToVal.size();
        int cnt = 0;
        std::string s = "(";

        auto it = objToVal.begin();
        for (; it != objToVal.end(); it++, cnt++)
        {
            if (cnt == size - 1)
                break;
            s = s + "[" + std::to_string(it->first.get_pid()) + ":" + std::to_string(it->first.get_index()) + "]";
            s = s + "(" + std::to_string(it->second) + ") ^ ";
        }

        s = s + "[" + std::to_string(it->first.get_pid()) + ":" + std::to_string(it->first.get_index()) + "]";
        s = s + "(" + std::to_string(it->second) + "))";
        return s;
    }

    bool sanity()
    {
        /* [rmnt]: Checking for various invariants that must be maintained
                   1. For every variable, we must have a corresponding value.
                   2. If the term has no free variables, it has either not been initialized yet (so result is INIT) or it must not have a DEPENDENT result. Conversely, if it doesn't have a dependent result, it must not have any redundant free variables.
                   3. Size of objects = Size of objToVal. Combined with 1, ensures one to one mapping between objToVal keys and objects
        */
        if (objects.size() != objToVal.size())
        {
            return false;
        }

        if (objects.empty())
        {
            if (result == RESULT::DEPENDENT)
            {
                return false;
            }
            else if ((!initPhase) && result == RESULT::INIT)
            {
                return false;
            }
            else if (initPhase && (result != RESULT::INIT))
            {
                return false;
            }
        }

        if ((result == RESULT::TRUE || result == RESULT::FALSE) && (!objects.empty()))
        {
            return false;
        }

        for (auto i = objects.begin(); i != objects.end(); i++)
        {
            if (objToVal.find(*i) == objToVal.end())
            {
                return false;
            }
        }
        return true;
    }

    friend bool operator==(const ProductTerm &t1, const ProductTerm &t2)
    {
        if (t1.objects == t2.objects)
        {
            for (auto i = t1.objects.begin(); i != t1.objects.end(); i++)
            {
                if (t1.objToVal.at(*i) != t2.objToVal.at(*i))
                {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
};

// [rmnt]: Forbidden values for a set of variables are represented as an SOP Formula. Each product term specifices one forbidden value per variable
template <class T>
class SOPFormula
{
public:
    std::forward_list<ProductTerm<T>> terms;
    RESULT result = RESULT::INIT;
    bool initPhase = true;

    SOPFormula() {}

    SOPFormula(std::pair<T, int> t)
    {
        ProductTerm<T> pt(t);
        addTerm(pt);
    };

    ~SOPFormula()
    {
        terms.clear();
    }

    // [rmnt]: Returns true if the term was not already there before, and false otherwise.
    //         Must ensure when we use this that terms using variables whose values have already been instantiated are not added. Otherwise result may have a mismatch
    bool addTerm(ProductTerm<T> &term)
    {
        assert(sanity());
        initPhase = false;
        if (result == RESULT::TRUE)
        {
            assert(sanity());
            return false;
        }
        for (auto i = terms.begin(); i != terms.end(); i++)
        {
            if (term == (*i))
            {
                assert(sanity());
                return false;
            }
        }
        terms.push_front(term);

        // [rmnt]: Update result
        if (!term.objects.empty())
        {
            assert(result != RESULT::TRUE);
            result = RESULT::DEPENDENT;
        }
        else
        {
            // [rmnt]: Ensure terms are initialized before being added to a formula
            assert(term.result == RESULT::FALSE || term.result == RESULT::TRUE);
            if (term.result == RESULT::TRUE)
            {
                terms.clear();
                result = RESULT::TRUE;
            }
            else
            {
                terms.pop_front();
                if (result == RESULT::INIT)
                {
                    result = RESULT::FALSE;
                }
            }
        }
        assert(sanity());
        return true;
    }

    // [rmnt]: Given a setting of (some of) the variables, reduce as much as we can and update result
    RESULT evaluate(std::unordered_map<T, int> valueEnv)
    {
        assert(sanity());
        // [rmnt]: Evaluating an unitialized formula is meaningless
        assert(result != RESULT::INIT);
        assert(!initPhase);
        if (terms.empty())
        {
            assert(result == RESULT::TRUE || result == RESULT::FALSE);
            assert(sanity());
            return result;
        }

        bool isDependent = false;
        for (auto i = terms.begin(); i != terms.end(); i++)
        {
            RESULT ans = i->evaluate(valueEnv);
            if (ans == RESULT::TRUE)
            {
                result = RESULT::TRUE;
                terms.clear();
                assert(sanity());
                return RESULT::TRUE;
            }
            else if (ans == RESULT::DEPENDENT)
            {
                result = RESULT::DEPENDENT;
                isDependent = true;
            }
        }

        if (!isDependent)
        {
            terms.clear();
            result = RESULT::FALSE;
            assert(sanity());
            return result;
        }

        assert(result == RESULT::DEPENDENT);

        // [rmnt]: Clean up false terms
        //         TODO should we clean up duplicate terms?
        terms.remove_if([](ProductTerm<T> t)
                        { return (t.result == RESULT::FALSE); });

        assert(sanity());

        return result;
    }

    RESULT check_evaluation(std::unordered_map<T, int> valueEnv)
    {
        RESULT ret_result = result;
        assert(sanity());
        // [rmnt]: Evaluating an unitialized formula is meaningless
        assert(ret_result != RESULT::INIT);
        assert(!initPhase);
        if (terms.empty())
        {
            assert(ret_result == RESULT::TRUE || ret_result == RESULT::FALSE);
            assert(sanity());
            return ret_result;
        }

        bool isDependent = false;
        for (auto i = terms.begin(); i != terms.end(); i++)
        {
            RESULT ans = i->check_evaluation(valueEnv);
            if (ans == RESULT::TRUE)
            {
                ret_result = RESULT::TRUE;
                assert(sanity());
                return RESULT::TRUE;
            }
            else if (ans == RESULT::DEPENDENT)
            {
                ret_result = RESULT::DEPENDENT;
                isDependent = true;
            }
        }

        if (!isDependent)
        {
            ret_result = RESULT::FALSE;
            assert(sanity());
            return ret_result;
        }

        assert(ret_result == RESULT::DEPENDENT);
        assert(sanity());

        return ret_result;
    }

    std::pair<bool, ProductTerm<T>> term_of_object(T object)
    {
        for (auto it = terms.begin(); it != terms.end(); it++)
        {
            if (it->is_term_of_object(object))
                return std::make_pair(true, (*it));
        }

        ProductTerm<T> dummy;
        return std::make_pair(false, dummy);
    }

    bool in(std::pair<T, int> term)
    {
        for (auto it = terms.begin(); it != terms.end(); it++)
        {
            if (it->unit() && it->in(term))
                return true;
        }

        return false;
    }

    friend void operator||(SOPFormula &f1, SOPFormula &f2)
    {
        SOPFormula f;
        f = f1;
        for (auto j = f2.terms.begin(); j != f2.terms.end(); j++)
        {
            f1.addTerm(*j);
        }
    }

    friend void operator||(SOPFormula &f1, std::pair<T, int> t)
    {
        ProductTerm<T> pt(t);
        f1.addTerm(pt);
    }

    friend void operator||(SOPFormula &f1, ProductTerm<T> term)
    {
        f1.addTerm(term);
    }

    void operator=(SOPFormula f)
    {
        terms = f.terms;
        result = f.result;
        initPhase = f.initPhase;
    }

    void clear()
    {
        terms.clear();
        result = RESULT::INIT;
    }

    std::string to_string()
    {
        if (result == RESULT::INIT || result == RESULT::FALSE)
            return "-";
        if (result == RESULT::TRUE)
            return "TRUE";

        std::string s = "(";

        auto it = terms.begin();
        for (; it != terms.end(); it++)
        {
            auto it1 = it;
            it1++;
            if (it1 == terms.end())
                break;
            s = s + it->to_string() + " v ";
        }

        s = s + it->to_string() + ")";
        return s;
    }

    SOPFormula AND(const SOPFormula &f2) const
    {
        assert(sanity() && f2.sanity());
        SOPFormula f;
        for (auto i = terms.begin(); i != terms.end(); i++)
        {
            for (auto j = f2.terms.begin(); j != f2.terms.end(); j++)
            {
                ProductTerm<T> disjunct = *i;
                bool toDiscard = false;
                for (auto k = j->objects.begin(); k != j->objects.end(); k++)
                {
                    if (disjunct.objects.find(*k) != disjunct.objects.end())
                    {
                        if (disjunct.objToVal.at(*k) == j->objToVal.at(*k))
                        {
                            continue;
                        }
                        else
                        {
                            toDiscard = true;
                        }
                    }
                    else
                    {
                        disjunct.addConstraint(*k, j->objToVal.at(*k));
                    }
                }
                if (!toDiscard)
                {
                    f.addTerm(std::ref(disjunct));
                }
            }
        }

        assert(f.sanity());
        return f;
    }

    bool sanity()
    {
        /* [rmnt]: Checking for various invariants which must be maintained:
         * 1. All the terms must themselves be "sane".
         * 2. None of the terms should be redundant, i.e., if there is a term still in the list, it must have a DEPENDENT result.
         * 3. There are only 2 cases in which the terms can be empty. First, if the formula hasn't been initialized yet. Second, if it has already evaluated to true or false.
         *    Conversely, if it has evaluated to true or false, there should be no redundant terms.
        */
        for (auto i = terms.begin(); i != terms.end(); i++)
        {
            if (!i->sanity())
            {
                return false;
            }
            if (i->result != RESULT::DEPENDENT)
            {
                return false;
            }
        }

        if (terms.empty())
        {
            if (result == RESULT::DEPENDENT)
            {
                return false;
            }
            else if ((!initPhase) && result == RESULT::INIT)
            {
                return false;
            }
            else if (initPhase && (result != RESULT::INIT))
            {
                return false;
            }
        }
        else
        {
            if (initPhase)
            {
                return false;
            }
            if (result != RESULT::DEPENDENT)
            {
                return false;
            }
        }
        return true;
    }
};

#endif