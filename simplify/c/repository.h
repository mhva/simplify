/*
   Copyright (C) 2010 Anton Mihalyov <anton@bytepaper.com>

   This  library is  free software;  you can  redistribute it  and/or
   modify  it under  the  terms  of the  GNU  Library General  Public
   License  (LGPL)  as published  by  the  Free Software  Foundation;
   either version  2 of the  License, or  (at your option)  any later
   version.

   This library  is distributed in the  hope that it will  be useful,
   but WITHOUT  ANY WARRANTY;  without even  the implied  warranty of
   MERCHANTABILITY or FITNESS  FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy  of the GNU Library General Public
   License along with this library; see the file COPYING.LIB. If not,
   write to the  Free Software Foundation, Inc.,  51 Franklin Street,
   Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef SIMPLIFY_REPOSITORY_H_
#define SIMPLIFY_REPOSITORY_H_

#define SIMPLIFY_DICT_UNKNOWN 0
#define SIMPLIFY_DICT_EPWING  1

#include <simplify/c/dictionary.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct simplify_repository simplify_repository_t;

/**
 * Tries to guess dictionary type.
 *
 * Dictionary types:
 * \li \e SIMPLIFY_DICT_UNKNOWN Unknown dictionary.
 * \li \e SIMPLIFY_DICT_EPWING  EPWING dictionary.
 *
 * \note Currently, all supported dictionary formats can be detected
 *  reliably.
 *
 * \return Returns dictionary type. If the method fails to detect dictionary
 *  type, SIMPLIFY_DICT_UNKNOWN will be returned.
 */
int simplify_repository_identify(const char *path);

/**
 * Creates or restores dictionary repository.
 *
 * \param repository_path Directory where repository should be initialized.
 *  The directory must exist and must be accessible by the current user.
 *
 * \note If the repository at this given directory doesn't exist, it will
 *  be created. In this case user must have read/write permissions for
 *  this directory.
 *
 * \note It is not advisable to create repository in the directory where
 *  some files do already exist. The function will not fail just because of
 *  this, but beware, creating repository in a non-empty directory might
 *  lead to a loss of some data.
 */
simplify_repository_t *simplify_repository_new(const char *repository_path);

/**
 * Destroys repository object and frees the memory that belongs to it.
 */
void simplify_repository_destroy(simplify_repository_t *sr);

/**
 * Creates dictionary object and registers it in the repository.
 *
 * \param name Name of the dictionary. Dictionary name will be used when
 *  initializing files and directories in the repository. Make sure that
 *  the name doesn't contain special characters forbidden to use by the
 *  underlying filesystem.
 * \param path Path to the dictionary file/files.
 * \param type Specifies dictionary type. If the dictionary type is not
 *  known in advance, it is possible to detect it with the
 *  simplify_repository_identify() function call.
 *
 * \note The fetch pointer will be reset after calling this function.
 *
 * \return If succeeds, returns a valid pointer to dictionary object.
 *  If fails, returns NULL and stores error code in the simplify_errno.
 */
simplify_dictionary_t *
simplify_repository_new_dictionary(simplify_repository_t *sr,
                                   const char *name,
                                   const char *path,
                                   int type);

/**
 * Unregisters dictionary object from the repository and destroys it.
 *
 * \note After calling this function user scripts will be left intact,
 *  but dictionary configuration will be completely erased.
 *
 * \note The fetch pointer will be reset after calling this function.
 */
void simplify_repository_delete_dictionary(simplify_repository_t *sr,
                                           simplify_dictionary_t *sd);

/**
 * Call this function to start enumerating dictionaries all over again.
 *
 * Example code (enumerate all dictionaries in repository):
 *  void enum_dicts(simplify_repository_t *repo) {
 *      simplify_dictionary_t *dict;
 *
 *      simplify_repository_begin_fetch(repo);
 *      while ((dict = simplify_repository_fetch_one(repo))) {
 *          [Useful Payload]
 *      }
 *  }
 */
void simplify_repository_begin_fetch(simplify_repository_t *sr);

/**
 * Fetches one dictionary from the repository. Use this function in couple
 * with @simplify_repository_begin_fetch() to enumerate dictionaries in
 * repository.
 *
 * See documentation of the @simplify_repository_begin_fetch() function for
 * example usage.
 *
 * \return Returns a pointer to dictionary object or NULL if there are no
 *  more dictionaries left to fetch.
 */
simplify_dictionary_t *
simplify_repository_fetch_one(simplify_repository_t *sr);

#ifdef __cplusplus
}
#endif

#endif  /* SIMPLIFY_REPOSITORY_H_ */
