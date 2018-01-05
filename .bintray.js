{
    "package": {
        "name": "roboschool",
        "repo": "roboschool",
        "subject": "openai",
        "vcs_url": "https://travis-ci.org/TheCrazyT/roboschool",
        "licenses": ["MIT"],
    },

    "version": {
        "name": "0.1",
    },

    "files":
        [
        {"includePattern": "roboschool/(.*)","excludePattern": ".*/cpp-household/.*", "uploadPattern": "$1"},
        ],
    "publish": true
}