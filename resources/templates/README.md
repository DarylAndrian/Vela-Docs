# Vela Docs v0.3.4 templates
#
# Each template is a JSON file plus an HTML preview thumbnail. The welcome
# screen reads `resources/templates/index.json` and renders the cards.
#
# Templates shipped at v0.3.4 (POLISH-02 partial):
#   1.  blank.json           — empty styled page
#   2.  modern_resume.json   — ATS-friendly modern resume
#   3.  classic_resume.json  — serif traditional resume
#   4.  creative_resume.json — sidebar creative resume
#   5.  formal_letter.json   — block business letter
#   6.  informal_letter.json — casual personal letter
#   7.  cover_letter.json    — targeted cover letter
#   8.  memo.json            — internal company memo
#   9.  report_toc.json      — long report with auto-TOC
#   10. invoice.json         — line-item invoice
#   11. newsletter.json      — single-page newsletter
#   12. flyer.json           — 1-page flyer
#   13. brochure.json        — trifold brochure
#   14. research_paper.json  — academic paper with abstract
#   15. thesis.json          — graduate-thesis skeleton
#   16. lab_report.json      — STEM lab report
#   17. essay.json           — MLA-style essay
#   18. meeting_minutes.json — agenda + minutes
#   19. budget.json          — household budget worksheet
#   20. todo_list.json       — printable to-do list
#
# Each .json has this shape:
# {
#   "id": "modern_resume",
#   "title": "Modern Resume",
#   "category": "Resumes",
#   "thumbnail": "modern_resume.png",
#   "preview":   "A clean two-column resume...",
#   "variables": ["{{name}}", "{{email}}", "{{phone}}"],
#   "body": { ... ProseMirror JSON ... }
# }
