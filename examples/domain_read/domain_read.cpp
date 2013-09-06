#include <iostream>
#include <string>
#include "DomainCollector.hpp"

using namespace DCollector;

/**
 * This libSplash example demonstrates on how to use the DomainCollector class
 * to read multiple, domain-annotated libSplash files transparently as a single
 * file.
 * 
 * The program expects the base part to a distributed libSplash file, i.e.
 * 'my_data', given that you have files like 'my_data_0_0_0.h5', ...
 */

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <libsplash-file-base>" << std::endl;
        return -1;
    }

    // libSplash filename
    std::string filename;
    filename.assign(argv[1]);

    // create DomainCollector
    DomainCollector dc(100);
    DataCollector::FileCreationAttr fAttr;
    DataCollector::initFileCreationAttr(fAttr);

    fAttr.fileAccType = DataCollector::FAT_READ_MERGED;

    dc.open(filename.c_str(), fAttr);

    // get number of entries
    int32_t *ids = NULL;
    size_t num_ids = 0;
    dc.getEntryIDs(NULL, &num_ids);

    if (num_ids == 0)
    {
        dc.close();
        return 1;
    } else
    {
        ids = new int32_t[num_ids];
        dc.getEntryIDs(ids, &num_ids);
    }

    // get entries for 1. id (timestep)
    DataCollector::DCEntry *entries = NULL;
    size_t num_entries = 0;
    dc.getEntriesForID(ids[0], NULL, &num_entries);

    if (num_entries == 0)
    {
        delete[] ids;
        dc.close();
        return 1;
    } else
    {
        entries = new DataCollector::DCEntry[num_entries];
        dc.getEntriesForID(ids[0], entries, &num_entries);
    }

    // read 1. entry
    DataCollector::DCEntry first_entry = entries[0];
    std::cout << "reading entry " << first_entry.name << std::endl;

    // read complete domain
    Domain domain = dc.getTotalDomain(ids[0], first_entry.name.c_str());
    DomainCollector::DomDataClass dataClass = DomainCollector::UndefinedType;
    DataContainer* container = dc.readDomain(ids[0], first_entry.name.c_str(),
            domain.getStart(), domain.getSize(), &dataClass, false);

    // access all elements
    for (size_t i = 0; i < container->getNumElements(); ++i)
    {
        void *element = container->getElement(i);
        // do anything with this element
        // ...
    }

    // POLY data might be distributed over multiple subdomains
    for (size_t d = 0; d < container->getNumSubdomains(); ++d)
    {
        DomainData* subdomain = container->getIndex(d);
        Dimensions size = subdomain->getSize();

        std::cout << "subdomain " << d << " has size " << size.toString() << std::endl;
    }

    // don't forget to delete the container allocated by DomainCollector
    delete container;

    delete[] entries;
    delete[] ids;

    dc.close();

    return 0;
}
