-- cbook.lua - the one piece of the design that TeX macros cannot do cleanly.
--
-- \code{...} typesets C exactly as it is written in the lectures. \detokenize gets most of the
-- way, but leaves two artifacts that C runs into constantly: it doubles every #, so #include
-- would print as ##include, and it puts a space after every control word, so "\n" would print as
-- "\n ". Both are undone here, and a line break is allowed after a comma with no space behind
-- it, so an expression like (1U<<CS10)|(1U<<CS12) can break rather than run into the margin (and
-- after a ::, which C never writes but costs nothing to keep).

local catcode_other = -2

-- LuaTeX allows a line break after every hyphen, whatever the font or the language says, and in C
-- a hyphen is an operator: led->vptr would break as led- and >vptr. A hyphen set in a box of its
-- own is out of the line breaker's reach.
local function emit(chunk)
  local start = 1
  while true do
    local i = chunk:find("-", start, true)
    if not i then break end
    if i > start then tex.sprint(catcode_other, chunk:sub(start, i - 1)) end
    tex.sprint("\\hbox{")
    tex.sprint(catcode_other, "-")
    tex.sprint("}")
    start = i + 1
  end
  if start <= #chunk then tex.sprint(catcode_other, chunk:sub(start)) end
end

function cbook_code(s)
  s = s:gsub("#+", "#")
  s = s:gsub("(\\%a+) ", "%1")
  -- \%, \{ and \} are how a literal percent sign or an unbalanced brace has to be written inside
  -- a TeX argument, \# is how a # has to be written in a heading or a caption, and \\ is a
  -- lone backslash.
  s = s:gsub("\\([%%{}#\\])", "%1")
  -- A break is allowed after a :: and after a comma with no space behind it; a comma followed by
  -- a space needs nothing, because the space is a breakpoint already. Each tex.sprint is read as a
  -- line of its own and TeX skips the spaces a line starts with, so no chunk may begin with one.
  local start = 1
  while true do
    local i, j = s:find("::", start, true)
    local k = s:find(",[^ ]", start)
    if k and (not i or k < i) then i, j = k, k end
    if not i then break end
    emit(s:sub(start, j))
    -- Discouraged rather than free, so a line breaks at a space when it can.
    tex.sprint("\\penalty100 ")
    start = j + 1
  end
  emit(s:sub(start))
end
