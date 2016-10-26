/**
 * Copyright 2015-2016 Carlchristian Eckert, Alexander Grund
 *
 * This file is part of libSplash.
 *
 * libSplash is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
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

#ifndef GENERATE_COLLECTION_TYPE_H
#define GENERATE_COLLECTION_TYPE_H

namespace splash
{

class CollectionType;

/**
 * Creates a new instance of a CollectionType based on the given datatype_id
 *
 * @param datatype_id the H5 datatype_id that should be converted into a
 *                    CollectionType
 *
 * @return A pointer to a heap-allocated CollectionType.
 *         The allocated object must be freed by the caller at the end of its
 *         lifetime.
 *         If no matching CollectionType was found, returns a ColTypeUnknown
 *         instance.
 */
CollectionType* generateCollectionType(hid_t datatype_id);

} /* namespace splash */


#endif /* GENERATE_COLLECTION_TYPE_H */
