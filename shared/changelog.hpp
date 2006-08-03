#ifndef GRAEHL__SHARED__CHANGELOG_HPP
#define GRAEHL__SHARED__CHANGELOG_HPP

#include <string>
#include <vector>

namespace graehl {

struct changelog
{
    typedef std::string description_t;
    typedef std::string date_t; // in YYYY.MM.DD format
    typedef double version_t;
    struct change
    {
        version_t version;
        date_t date;
        description_t description;
        change(version_t version, date_t const& date, description_t const& description) : version(version), date(date), description(description) {}
        template <class O>
        void print(O &o) const
        {
            o << "Version "<<version;
            if (!date.empty())
                o <<" ("<<date<<")";
            o<<": "<<description<<"\n";
        }
    };
    
    typedef std::vector<change> changes_t;
    std::string product;
    changes_t changes;
    
    changelog &operator()(std::string const& prod) 
    {
        product=prod;
        return *this;
    }
    
    changelog & operator()(version_t version, description_t const& description,date_t const& date="")
    {
        changes.push_back(change(version,date,description));
        return *this;
    }
    version_t max_version() const
    {
        version_t max=0;
        for (changes_t::const_iterator i=changes.begin(),e=changes.end();i!=e;++i)
            if (i->version > max)
                max=i->version;
        return max;
    }
    version_t versions_ago(version_t ago) const
    {
        return max_version()-ago;
    }

    // negative since => ago
    template <class O>
    void print(O &o,version_t since_version=0) const
    {
        if (since_version < 0)
            since_version=versions_ago(-since_version);
        o << product<<" changes since version " <<since_version<<":\n";
        for (changes_t::const_iterator i=changes.begin(),e=changes.end();i!=e;++i)
            if (i->version >= since_version)
                i->print(o);
    }   
};

}


#endif
