all

# Allow mixing of header styles, within reason.
# I use "setext" style when I can, but have to switch to "atx" (hash marks)
# for the more fine-grained sections which setext does not support.
rule 'MD003', :style => :setext_with_atx

# Multiple consecutive blank lines.  Sorry, but I want my Markdown to look
# nice.  And there does not seem to be an option for "allow a maximum of 2."
exclude_rule 'MD012'

# What joker came up with this?  There may be some sense to warning about
# trailing punctuation, but not when it's a question mark or exclamantion mark!
rule 'MD026', :punctuation => '.,;:'

# In an ordered list, I like to start each item with the right number, not with
# "1."
rule 'MD029', :style => :ordered

# Allow bare URLs.  Hate to have to disable this, but mainpage.md has some
# special syntax which requires bare URLs.
exclude_rule 'MD034'
