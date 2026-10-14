import unittest
from tools.checkpoint import quota_windows

class QuotaTests(unittest.TestCase):
    def test_prefer_current_bucket(self):
        data={'rateLimitsByLimitId':{'codex':{'primary':{'windowDurationMins':300,'usedPercent':95},
                                             'secondary':{'windowDurationMins':10080,'usedPercent':13}}},
              'rateLimits':{'primary':{'windowDurationMins':300,'usedPercent':1}}}
        result=quota_windows(data)
        self.assertEqual(result['five_hour']['remaining'],5)
        self.assertEqual(result['weekly']['remaining'],87)
    def test_unknown_is_not_zero(self):
        self.assertEqual(quota_windows({'rateLimits':{'primary':{'windowDurationMins':300,'usedPercent':None}}}),{})
        self.assertEqual(quota_windows({}),{})
    def test_do_not_invent_windows(self):
        self.assertEqual(quota_windows({'rateLimits':{'primary':{'windowDurationMins':60,'usedPercent':99}}}),{})
