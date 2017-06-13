# QPack-C
Fast and efficient data serializer for the C program language.

---------------------------------------
  * [Installation](#installation)
  * [Usage](#usage)

---------------------------------------

## Installation
Install debug or release version, in this example we will install the release version.
```
$ cd Release
```

Compile qpack
```
$ make all
```

Install qpack
```
$ sudo make install
```

> Note: run `sudo make uninstall` for removal.

## Usage
This is a simple example of how to use qpack.
```c
#include <qpack.h>

int main(void)
{
    qp_packer_t * packer = qp_packer_create(QP_SUGGESTED_SIZE);
    if (packer == NULL) {
        abort();
    }
    return 0;
}
```


