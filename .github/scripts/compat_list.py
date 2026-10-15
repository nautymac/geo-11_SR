#!/usr/bin/env python3
"""Builds the 3D compatibility list from the issues labelled 'compat' (the "3D compatibility report" form).

Output, grouped by engine and then by game:
  COMPATIBILITY.md     English list
  COMPATIBILITY.ko.md  Korean list
  compat.json          the same data for the installer (game list column, report button state)

Run by .github/workflows/compat-list.yml. Local test:
  python compat_list.py --fixture issues.json --out OUTDIR
"""
import argparse
import json
import os
import re
import sys
import urllib.request

REPO_URL = "https://github.com/{repo}"
EXCLUDE_LABELS = {"invalid", "duplicate", "spam", "wontfix"}
NO_RESPONSE = "_No response_"

# key, English heading, Korean heading, pattern on the form's engine field (first match wins)
ENGINES = [
    ("Unity", "Unity (Universal Fix)", "Unity (Universal Fix)", r"unity"),
    ("UE4", "Unreal Engine 4 (Universal Fix 2)", "Unreal Engine 4 (Universal Fix 2)", r"ue\s*-?\s*4|unreal"),
    ("DX9", "DirectX 9 (dgVoodoo2 + geo-11)", "DirectX 9 (dgVoodoo2 + geo-11)", r"dx\s*9|directx\s*9|d3d\s*9|dgvoodoo"),
    ("OpenGL", "OpenGL (OGL3D)", "OpenGL (OGL3D)", r"open\s*gl|ogl"),
    ("DX11", "DirectX 11 (generic geo-11)", "DirectX 11 (일반 geo-11)", r"dx\s*1[01]|directx\s*1[01]|d3d\s*1[01]|generic|preferred|일반"),
]
OTHER = ("Other", "Other / unknown engine", "기타 / 엔진 미상")

# result key, pattern on the form's result field (checked in this order: "Does not work" also contains "work")
RESULTS = [("broken", r"does\s*not|doesn.?t|not\s*work|안\s*됨"), ("issues", r"issue|problem|문제"), ("works", r"works|잘|가능")]
RESULT_TEXT = {
    "en": {"works": "✅ Works well", "issues": "⚠️ Works with issues", "broken": "❌ Does not work"},
    "ko": {"works": "✅ 잘 됨", "issues": "⚠️ 문제 있음", "broken": "❌ 안 됨"},
}
ICON = {"works": "✅", "issues": "⚠️", "broken": "❌"}


def fetch_issues(repo, token):
    issues, page = [], 1
    while True:
        url = "https://api.github.com/repos/%s/issues?labels=compat&state=all&per_page=100&page=%d" % (repo, page)
        req = urllib.request.Request(url, headers={"Accept": "application/vnd.github+json", "User-Agent": "geo-11-sr-compat-list"})
        if token:
            req.add_header("Authorization", "Bearer " + token)
        with urllib.request.urlopen(req) as r:
            batch = json.loads(r.read().decode("utf-8"))
        issues += [i for i in batch if "pull_request" not in i]
        if len(batch) < 100:
            return issues
        page += 1


def sections(body):
    """'### Heading' + value blocks of an issue form body -> {field: value}."""
    out = {}
    parts = re.split(r"^###\s+(.+?)\s*$", (body or "").replace("\r\n", "\n"), flags=re.M)
    for i in range(1, len(parts) - 1, 2):
        head, val = parts[i].strip().lower(), parts[i + 1].strip()
        if val == NO_RESPONSE:
            val = ""
        for field, words in (("game", ("game", "게임")), ("engine", ("engine", "엔진")), ("result", ("result", "결과")),
                             ("details", ("setup", "설정")), ("notes", ("notes", "메모"))):
            if any(head.startswith(w) for w in words) and field not in out:
                out[field] = val
                break
    return out


def engine_of(text):
    t = (text or "").lower()
    for key, _en, _ko, pat in ENGINES:
        if re.search(pat, t):
            return key
    return OTHER[0]


def result_of(text):
    t = (text or "").lower()
    for key, pat in RESULTS:
        if re.search(pat, t):
            return key
    return None


def details_of(text):
    """'Key: value' lines of the installer's setup block -> {key: value}."""
    d = {}
    for line in (text or "").splitlines():
        m = re.match(r"^\s*[-*]?\s*([A-Za-z0-9 /().-]+?)\s*:\s*(.+?)\s*$", line)
        if m:
            d.setdefault(m.group(1).lower(), m.group(2))
    return d


def game_key(name):
    n = re.sub(r"[™®©]", "", (name or "").lower())
    return re.sub(r"[\W_]+", "", n)


def cell(text, limit=120):
    """Untrusted issue text -> one safe Markdown table cell."""
    t = re.sub(r"\s+", " ", text or "").strip()
    if len(t) > limit:
        t = t[: limit - 1].rstrip() + "…"
    t = t.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    return re.sub(r"([\\`*_\[\]|#!])", r"\\\1", t)


def parse(issue):
    labels = {((l.get("name") if isinstance(l, dict) else l) or "").lower() for l in issue.get("labels", [])}
    if labels & EXCLUDE_LABELS or issue.get("state_reason") == "not_planned":
        return None
    f = sections(issue.get("body"))
    game = (f.get("game") or "").strip()
    result = result_of(f.get("result"))
    if not game or not result:
        return None
    det = details_of(f.get("details"))
    steam = re.search(r"steam\D*(\d+)", det.get("store", ""), re.I)
    exe = det.get("executable", "").strip().lower()
    return {
        "number": issue["number"], "url": issue.get("html_url", ""), "created": issue.get("created_at", ""),
        "game": game, "key": game_key(game), "engine": engine_of(f.get("engine") or det.get("engine")),
        "result": result, "bits": re.sub(r"\D", "", det.get("bits", "")), "notes": f.get("notes", ""),
        "steam": steam.group(1) if steam else "", "exe": exe if exe.endswith(".exe") else "",
    }


def aggregate(reports):
    games = {}
    for r in sorted(reports, key=lambda r: (r["created"], r["number"])):
        g = games.setdefault((r["engine"], r["key"]), {"engine": r["engine"], "key": r["key"], "reports": [],
                                                       "works": 0, "issues": 0, "broken": 0, "bits": set(),
                                                       "steam": set(), "exe": set(), "note": ""})
        g["reports"].append(r)
        g[r["result"]] += 1
        g["game"] = r["game"]            # newest report's spelling
        g["last"] = r["result"]
        g["updated"] = r["created"][:10]
        if r["bits"]:
            g["bits"].add(r["bits"])
        if r["steam"]:
            g["steam"].add(r["steam"])
        if r["exe"]:
            g["exe"].add(r["exe"])
        if r["notes"]:
            g["note"] = r["notes"]
    for g in games.values():
        counts = {k: g[k] for k in ("works", "issues", "broken")}
        best = max(counts.values())
        tied = [k for k, v in counts.items() if v == best]
        g["status"] = g["last"] if g["last"] in tied else tied[0]   # majority, the newest report breaks a tie
    return games


def render(games, lang, repo, total):
    form = REPO_URL.format(repo=repo) + "/issues/new?template=compat_report.yml"
    ko = lang == "ko"
    L = []
    if ko:
        L += ["# 3D 호환 목록", "",
              "사용자 보고로 자동으로 만들어지는 목록입니다. 엔진별, 게임별로 정리합니다. "
              "보고는 설치기의 **[3D 동작 보고...]** 버튼이나 [보고 양식](%s)으로 합니다." % form, "",
              "✅ 잘 됨 · ⚠️ 문제 있음 · ❌ 안 됨. 상태는 보고의 다수결이고, 같으면 최근 보고를 따릅니다.", "",
              "게임 %d개 · 보고 %d건 · English: [COMPATIBILITY.md](COMPATIBILITY.md)" % (len(games), total), ""]
        head = "| 게임 | 상태 | 보고 | 비트 | 최근 메모 | 이슈 |"
    else:
        L += ["# 3D compatibility list", "",
              "Built automatically from user reports, grouped by engine and then by game. "
              "Report a game with the installer's **[Report 3D...]** button or the [report form](%s)." % form, "",
              "✅ works well · ⚠️ works with issues · ❌ does not work. The status is the majority of the reports; a tie goes to the newest report.", "",
              "%d games · %d reports · 한국어: [COMPATIBILITY.ko.md](COMPATIBILITY.ko.md)" % (len(games), total), ""]
        head = "| Game | Status | Reports | Bits | Latest note | Issues |"
    for key, en, kt in [(k, en, kt) for k, en, kt, _p in ENGINES] + [OTHER]:
        rows = sorted((g for g in games.values() if g["engine"] == key), key=lambda g: g["game"].lower())
        if key == OTHER[0] and not rows:
            continue
        L += ["## " + (kt if ko else en), ""]
        if not rows:
            L += ["_아직 보고가 없습니다._" if ko else "_No reports yet._", ""]
            continue
        L += [head, "|---|---|---|---|---|---|"]
        for g in rows:
            counts = " · ".join("%s %d" % (ICON[k], g[k]) for k in ("works", "issues", "broken") if g[k])
            links = " ".join("[#%d](%s)" % (r["number"], r["url"]) for r in g["reports"][-5:])
            L.append("| %s | %s | %s | %s | %s | %s |" % (cell(g["game"], 80), RESULT_TEXT[lang][g["status"]], counts,
                                                            "/".join(sorted(g["bits"])) or "-", cell(g["note"]) or "-", links))
        L.append("")
    return "\n".join(L)


def to_json(games):
    out = []
    for g in sorted(games.values(), key=lambda g: (g["engine"], g["game"].lower())):
        out.append({"engine": g["engine"], "game": g["game"], "key": g["key"], "status": g["status"],
                    "works": g["works"], "issues": g["issues"], "broken": g["broken"], "updated": g["updated"],
                    "bits": sorted(g["bits"]), "steam": sorted(g["steam"]), "exe": sorted(g["exe"]),
                    "reports": [r["number"] for r in g["reports"]]})
    return {"version": 1, "games": out}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", default=os.environ.get("GITHUB_REPOSITORY", "nautymac/geo-11_SR"))
    ap.add_argument("--out", default=".")
    ap.add_argument("--fixture", help="JSON file with issues (API format) instead of calling GitHub")
    ap.add_argument("--issue", type=int, default=int(os.environ.get("ISSUE_NUMBER") or 0), help="report the engine of this issue")
    a = ap.parse_args()
    if a.fixture:
        with open(a.fixture, encoding="utf-8") as f:
            issues = json.load(f)
    else:
        issues = fetch_issues(a.repo, os.environ.get("GITHUB_TOKEN"))
    reports = [r for r in map(parse, issues) if r]
    games = aggregate(reports)
    for name, text in (("COMPATIBILITY.md", render(games, "en", a.repo, len(reports))),
                       ("COMPATIBILITY.ko.md", render(games, "ko", a.repo, len(reports)))):
        with open(os.path.join(a.out, name), "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
    with open(os.path.join(a.out, "compat.json"), "w", encoding="utf-8", newline="\n") as f:
        json.dump(to_json(games), f, ensure_ascii=False, indent=1)
        f.write("\n")
    mine = next((r for r in reports if r["number"] == a.issue), None)
    print("%d reports from %d issues, %d games" % (len(reports), len(issues), len(games)))
    gh_out = os.environ.get("GITHUB_OUTPUT")
    if gh_out:
        with open(gh_out, "a", encoding="utf-8") as f:
            f.write("listed=%s\nengine=%s\n" % ("true" if mine else "false", mine["engine"] if mine else ""))


if __name__ == "__main__":
    sys.exit(main())
