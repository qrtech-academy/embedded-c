# The Book

The course typeset as a book with LuaLaTeX: ten chapters, one per lecture, then the simulator guide,
the answers to the exercises that have one right answer, and the two self-assessment papers as
appendices.

---

## Building it

```bash
sudo apt -y install make texlive-luatex texlive-latex-extra fonts-texgyre fonts-texgyre-math \
                    fonts-dejavu-core
make -C book                   # Writes book/embedded-c.pdf, dated today.
make -C book VERSION=book-v2   # The same, with the version on the title page.
make -C book clean             # Removes book/build/ and the PDF.
```

The build runs LuaLaTeX twice, so the contents and the cross-references settle, and a third time
when LaTeX asks for it. It then prints any overfull or underfull lines and LaTeX warnings it found,
and fails if a reference is left undefined.

Every chapter and appendix is an `\include`, so one of them can be built on its own while you work
on it. References into the parts left out print as `??`, and are not an error here:

```bash
make -C book partial ONLY=chapters/03/chapter,back/answers/answers
```

Set `BUILD=some/dir` to build somewhere other than `book/build`, for example to run two builds side
by side.

---

## Releasing a new edition

The PDF is committed, as `book/embedded-c.pdf`, so the repository always holds a readable copy;
rebuild it with `make -C book` and commit it along with any change to the book. Each edition is
also published as a GitHub release, with its version on the title page. Push a tag named
`book-v*`:

```bash
git tag book-v1
git push origin book-v1
```

The [Book workflow](../.github/workflows/book.yml) then builds the PDF with the tag on its title
page and attaches it to a release of the same name. The README's download link always points at
the newest release, so it never needs updating.

---

## What is where

```text
book.tex                The book: front matter, ten chapters, appendices, in order.
cbook.sty               Every visual decision: page, type, colours, code blocks, exercises.
cbook.lua               How \code{...} typesets inline C (#, \n and line breaks).
front/                  Title pages and preface.
chapters/NN/            Chapter NN: chapter.tex (the opener), one file per appendix of lecture
                        LNN, summary.tex (the review) and exercises.tex.
back/sim/               Appendix A: running the demos in simavr, from info/.
back/answers/           Appendix B: the answers to the Reflection and Calculation exercises,
                        answers.tex and one file per chapter.
back/exam/              Appendices C to G: the papers and their model answers.
```

Every `.tex` file typeset from course material starts with a comment naming its source, for
example:

```tex
% Section 3.1, from lectures/L03/appendix/a_interrupts.md.
```

---

## Updating the content

The course material is the source of truth, and the book follows it. **Two kinds of content behave
differently:**

* **The demos' files and the images update themselves.** The book does not contain the demos'
  code, their Makefiles, the circuit diagrams or the screenshots; it includes the files under
  `lectures/` directly (`\cfile{lectures/...}`, `\makefile{...}`, `\bookimage{...}`), so a change to
  one is in the book on the next build, with nothing to edit here.
* **Prose and the code snippets in the text do not.** A chapter's text is a typeset copy of its
  lecture's markdown. When you change a lecture appendix, make the same change in the `.tex` file
  whose header names it. The same holds for `info/simulating_with_simavr.md` and `back/sim/`, and
  for `exam/*.md` and `back/exam/`.

A few conventions, so an edit reads like the rest of the book:

* Code blocks: `ccode` (C), `cppcode` (the one Arduino sketch), `shell`, `makecode` (Makefiles;
  recipe lines keep their tab), and `console` (program output, directory trees, plain text). The
  first four are set by listings, which cannot place a character outside ASCII; `console` takes any
  character.
* Inline code: `\code{...}`, written exactly as in the source. Inside it, write `\%` for `%`,
  `\{` or `\}` for an unbalanced brace, `\\` for a backslash, and `\#` for `#` in a heading or
  caption.
* References: `\secref{c3:a:4}` is the section typeset from Appendix A.4 of L03, and
  `Exercise~\ref{c9:ex:5}` is exercise 5 of L09. The book numbers exercises within each chapter,
  in the lecture's own order, so Exercise 9.5 is L09's exercise 5.
* Exercises: `\exerciseset{Title}` for a set, then `\reflexercise{Title}`, `\calcexercise{Title}`
  or `\codeexercise{Title}` for one exercise, with `\label{cN:ex:M}` after it, and `\task{...}` for
  a titled part.
* Answers: `\answer{cN:ex:M}` in `back/answers/NN.tex` heads the answer to that exercise, with its
  number and title taken from the exercise. A new Reflection or Calculation exercise needs one; a
  Code exercise gets no printed solution, only answers to any questions it asks.
* A new lecture appendix is a new file in `chapters/NN/`, `\input` from that chapter's
  `chapter.tex`.

---

## License

The book, its text and the PDF built from it, is licensed under
[CC BY-NC-SA 4.0](../LICENSE-CONTENT), like the course material it is typeset from; the
third-party artwork in the figures remains subject to its owners' terms. This directory's build
files (`cbook.sty`, `cbook.lua`, `Makefile`) are released under the repository's
[MIT License](../LICENSE), like all the code in the course, and so may the code examples printed in
the book be used.
