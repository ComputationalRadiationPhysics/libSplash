/**
 * Copyright 2013 Felix Schmitt
 *
 * This file is part of libSplash. 
 * 
 * libSplash is free software: you can redistribute it and/or modify 
 * it under the terms of of either the GNU General Public License or 
 * the GNU Lesser General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version. 
 * libSplash is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License and the GNU Lesser General Public License 
 * for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * and the GNU Lesser General Public License along with libSplash. 
 * If not, see <http://www.gnu.org/licenses/>. 
 */



#ifndef DATACONTAINER_HPP
#define	DATACONTAINER_HPP

#include <vector>

#include "DCException.hpp"
#include "Dimensions.hpp"
#include "domains/DomainData.hpp"

namespace DCollector
{

    /**
     * Container for storing domain-annotated data representing a specific subdomain.
     * The container contains information on the specific subdomain and its global domain.
     * This should regularly not be constructed by a user but internally, only.
     */
    class DataContainer
    {
    public:

        /**
         * Constructor.
         */
        DataContainer() :
        offset(0, 0, 0),
        size(1, 1, 1)
        {
        }

        /**
         * Destructor.
         * 
         * Deletes all subdomains it contains.
         */
        virtual ~DataContainer()
        {
            for (std::vector<DomainData* >::iterator iter = subdomains.begin();
                    iter != subdomains.end(); ++iter)
            {
                if (*iter != NULL)
                    delete (*iter);
            }

            subdomains.clear();
        }

        /**
         * Internal use only!
         */
        void add(DomainData* entry)
        {
            if (entry == NULL)
                throw DCException("Entry in DataContainer must not be NULL.");

            if (entry->getData() == NULL)
                throw DCException("Data in entry in DataContainer must not be NULL.");

            const Dimensions &entryOffset = entry->getOffset();
            const Dimensions entryEnd = entry->getEnd();
            
            for (uint32_t i = 0; i < 3; ++i)
            {
                offset[i] = std::min(entryOffset[i], offset[i]);
                size[i] = std::max(entryEnd[i] - offset[i], size[i]);
            }

            subdomains.push_back(entry);
        }

        /**
         * Returns the number of subdomain partitions in the container.
         * 
         * @return Number of subdomain partitions.
         */
        size_t getNumSubdomains()
        {
            return subdomains.size();
        }

        /**
         * Returns the total number of elements in all subdomains of this container.
         * 
         * @return Total number of elements in this container.
         */
        size_t getNumElements()
        {
            size_t num_elements = 0;

            for (std::vector<DomainData* >::const_iterator iter = subdomains.begin();
                    iter != subdomains.end(); ++iter)
            {
                num_elements += (*iter)->getElements().getScalarSize();
            }

            return num_elements;
        }

        /**
         * Returns the size of the partition represented by this container.
         * 
         * @return Size of total domain partition in this container.
         */
        Dimensions getSize() const
        {
            return size;
        }
        
        /**
         * Returns the offset of the partition represented by this container.
         * 
         * @return Offset of total domain partition in this container.
         */
        Dimensions getOffset() const
        {
            return offset;
        }
        
        /**
         * Returns the end of the partition represented by this container
         * which is the combination of its offset and size.
         * 
         * @return End of total domain partition in this container.
         */
        Dimensions getEnd() const
        {
            return offset + size;
        }

        /**
         * Returns the pointer to the DomainData with the specified index.
         * 
         * @param index Index of subdomain partition.
         * @return Subdomain partition.
         */
        DomainData* getIndex(size_t index)
        {
            if (subdomains.size() > index)
                return subdomains[index];

            throw DCException("Invalid index in DataContainer");
        }

        /**
         * Returns a pointer to the DomainData with index (x) for 1-dimensional domains.
         * 
         * @param x Index of subdomain partition.
         * @return Subdomain partition.
         */
        DomainData* get(size_t x)
        {
            if (x < size[0])
                return subdomains[x];

            throw DCException("Invalid entry in DataContainer (1)");
        }

        /**
         * Returns a pointer to the DomainData with index (x, y) for 2-dimensional domains.
         * 
         * @param x First dimension index of subdomain partition.
         * @param y Second dimension index of subdomain partition.
         * @return Subdomain partition.
         */
        DomainData* get(size_t x, size_t y)
        {
            if (x < size[0] && y < size[1])
                return subdomains[y * size[0] + x];

            throw DCException("Invalid entry in DataContainer (2)");
        }

        /**
         * Returns a pointer to the DomainData with index (x, y, z) for 3-dimensional domains.
         * 
         * @param x First dimension index of subdomain partition.
         * @param y Second dimension index of subdomain partition.
         * @param z Third dimension index of subdomain partition.
         * @return Subdomain partition.
         */
        DomainData* get(size_t x, size_t y, size_t z)
        {
            if (x < size[0] && y < size[1] && z < size[2])
                return subdomains[z * size[0] * size[1] + y * size[0] + x];

            throw DCException("Invalid entry in DataContainer (3)");
        }

        /**
         * Returns a pointer to the data element with global index.
         * 
         * @param index Index among all elements in this container,
         * see \ref DataContainer::getNumElements.
         * @return Pointer to element.
         */
        void* getElement(size_t index)
        {
            if (subdomains.size() == 0)
                return NULL;

            size_t elements = 0;

            for (size_t i = 0; i < subdomains.size(); ++i)
            {
                DomainData *subdomain = subdomains[i];
                size_t subdomain_elements = subdomain->getElements().getScalarSize();

                if (elements + subdomain_elements > index)
                {
                    size_t type_size = subdomain->getTypeSize();
                    size_t local_index = index - elements;

                    assert(subdomain->getData() != NULL);
                    if (subdomain->getData() == NULL)
                        return NULL;

                    return (((uint8_t*) (subdomain->getData())) + (type_size * local_index));
                } else
                    elements += subdomain_elements;
            }

            return NULL;
        }

    private:
        std::vector<DomainData* > subdomains;
        Dimensions offset;
        Dimensions size;
    };

}

#endif	/* DATACONTAINER_HPP */

