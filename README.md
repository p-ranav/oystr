# oystr

`oystr` is a command-line tool that recursively searches directories for a substring.

* Fast case-sensitive search for string literal
* Recursive - ignores certain directories (e.g., `.git`, `build`) and file extensions (e.g., `.pdf`)
* Uses `AVX512F` or `AVX2` SIMD instructions if possible

<p align="center">
  <img height="200" src="images/demo.png"/>  
</p>

# Building and installing

See the [BUILDING](BUILDING.md) document.

# Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) document.

# Licensing

<!--
Please go to https://choosealicense.com/ and choose a license that fits your
needs. GNU GPLv3 is a pretty nice option ;-)
-->
