# Simplify Dictionary

Let's you read commercial dictionaries compiled into highly obscure **EPWING** format. While the format is no longer used for dictionaries coming out today, there's still a lot of old, high-quality commercial dictionaries that are still unavailable on a better digital medium out there.

# Features

 * Basic navigation
 * Highlight a word within an article to show its definition
 * Wildcard expressions in search queries
 * Customized article formatting with user-scripts written in JavaScript
 * Out-of-the-box formatting support for popular dictionaries

# Supported OS

 - [x] **Linux**
 - [ ] **macOS** (might compile and run, but not tested)
 - [ ] **Windows**

# Building
## Linux
The building process is a bit involved and takes some time. Here's a step-by-step guide:

```console
$ git clone https://github.com/mhva/simplify.git
$ cd simplify
```

Fetch third party dependencies using `bootstrap.py`. Make sure that `/usr/bin/python` points to your **Python 2** interpreter. Tools in **depot_tools** expect this to be the case and it's not something we can easily change.
```console
$ ./bootstrap.py fetch
```

Next step is to build the dependencies (this will take a while):
```console
$ ./bootstrap.py build v8
```

Now it's time to build the program itself, you will need to have **cmake** installed for this:
```console
$ mkdir ../simplify-build
$ cd ../simplify-build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release ../simplify
$ make
```

Note, change the value of `CMAKE_INSTALL_PREFIX`, if you want to install program elsewhere.

Finally, execute `make install` to install the program:
```console
$ sudo make install
```

# Configuration

Currently, there is no user interface for configuring your dictionaries, so you have to write config file manually. Here's an example:

```json
{
	"dicts": [
		{
			"name": "大辞泉",
			"type": "epwing",
			"path": "/path/to/dictionary1",
			"state": {
				"subbook": 0,
				"script": "/usr/local/share/simplify/daijisen.js"
			}
		},
		{
			"name": "大辞林",
			"type": "epwing",
			"path": "/path/to/dictionary2",
			"state": {
				"subbook": 0,
				"script": "/usr/local/share/simplify/daijirin.js"
			}
		}
	]
}
```

 - `dicts` is an array containing a descriptor for each user's dictionary.
 - `type` is a type of dictionary. Only **epwing** is supported at the moment.
 - `path` is dictionary's path. Note, that in **epwing** case, `path` must point to dictionary's directory, not its `CATALOGS` file or anything else.
 - `state` is dictionary's private state. Yes, it should be filled in as well, unfortunately. Below is the description of each key that is required by **epwing** dictionary:
   * `subbook` is a sub-book index. Most of the time it's `0`.
   * `script` is a path to custom user script. Can be empty.

The file should be saved to `$HOME/.config/simplify/repository.js` or, alternatively, it can be saved anywhere and it's path passed to **simplifyd** with `--repository` option.

# Running

Start `simplifyd` from terminal and point your web-browser to `http://127.0.0.1:8000`. See `simplifyd --help` for more options.

# TODO

 - [ ] Kanjidic integration
   - [ ] Search by radical
   - [ ] Handwriting recognition
   - [ ] Decompose character into radicals
 - [ ] GUI for managing dictionaries
 - [ ] Port to electron
 - [ ] Windows port