# oystr

`oystr` is a command-line tool that recursively searches directories for a substring.

* Fast case-sensitive search for string literal
* Recursive - ignores certain directories (e.g., `.git`, `build`) and file extensions (e.g., `.pdf`)
* Uses `AVX512F` or `AVX2` SIMD instructions if possible

<p align="center">
  <img height="200" src="images/demo.png"/>  
</p>
