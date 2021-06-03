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
    std::unordered_map<T, int> objToVal;
    bool initPhase = true;

public:
    std::unordered_set<T> objects;
    RESULT result = RESULT::INIT;

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
        terms.remove_if([](ProductTerm<T> t) {
            return (t.result == RESULT::FALSE);
        });

        assert(sanity());

        return result;
    }

    friend SOPFormula operator+(const SOPFormula &f1, const SOPFormula &f2)
    {
        SOPFormula f;
        f = f1;
        for (auto j = f2.terms.begjn(); j != f2.terms.end(); j++)
        {
            f.addTerm(*j);
        }
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
